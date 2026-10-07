# 용도와 실행 방법: README.md 의 "검증"
param(
    [string]$Exe = 'build\Debug\SoftPipe.exe',
    [int[]]$ClientSizes = @(1280, 720, 2560, 1440, 1000, 700, 1600, 500, 333, 517)
)

$ErrorActionPreference = 'Stop'

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class PresentationCheck
{
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }

    [DllImport("user32.dll")] static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
    [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr hwnd, IntPtr after, int x, int y, int w, int h, uint flags);
    [DllImport("user32.dll")] static extern bool PrintWindow(IntPtr hwnd, IntPtr dc, uint flags);
    [DllImport("user32.dll")] static extern bool PostMessageW(IntPtr hwnd, uint msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr hwnd, int cmd);
    [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr hwnd, ref POINT point);
    [DllImport("user32.dll")] static extern IntPtr MonitorFromWindow(IntPtr hwnd, uint flags);
    [DllImport("user32.dll")] static extern bool GetMonitorInfoW(IntPtr monitor, ref MONITORINFO info);

    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }

    [StructLayout(LayoutKind.Sequential)]
    public struct MONITORINFO { public int Size; public RECT Monitor; public RECT Work; public uint Flags; }

    const uint PW_CLIENTONLY_RENDERFULLCONTENT = 3;

    public static void UsePerMonitorV2Dpi()
    {
        SetThreadDpiAwarenessContext(new IntPtr(-4));
    }

    // 화면 밖으로 나간 client 영역은 capture 에 반영되지 않으므로 client 의 왼쪽 위를 모니터의 왼쪽 위에 맞춘다.
    // 결과: { client 폭, client 높이, client 가 모니터 안에 다 들어가면 1 }
    public static int[] PlaceClient(IntPtr hwnd, int width, int height)
    {
        RECT window, client;
        GetWindowRect(hwnd, out window);
        GetClientRect(hwnd, out client);
        POINT origin = new POINT();
        ClientToScreen(hwnd, ref origin);
        int borderWidth = (window.Right - window.Left) - client.Right;
        int borderHeight = (window.Bottom - window.Top) - client.Bottom;

        MONITORINFO info = new MONITORINFO();
        info.Size = Marshal.SizeOf(typeof(MONITORINFO));
        GetMonitorInfoW(MonitorFromWindow(hwnd, 2), ref info);

        int x = info.Monitor.Left - (origin.X - window.Left);
        int y = info.Monitor.Top - (origin.Y - window.Top);
        SetWindowPos(hwnd, IntPtr.Zero, x, y, width + borderWidth, height + borderHeight, 0x0004 | 0x0010);

        GetClientRect(hwnd, out client);
        origin = new POINT();
        ClientToScreen(hwnd, ref origin);
        bool fits = origin.X >= info.Monitor.Left && origin.Y >= info.Monitor.Top
            && origin.X + client.Right <= info.Monitor.Right && origin.Y + client.Bottom <= info.Monitor.Bottom;
        return new int[] { client.Right, client.Bottom, fits ? 1 : 0 };
    }

    public static void Minimize(IntPtr hwnd) { ShowWindow(hwnd, 6); }
    public static void Restore(IntPtr hwnd) { ShowWindow(hwnd, 9); }

    public const int VK_PAUSE = 0x13;
    public const int VK_F9 = 0x78;

    public static void PressKey(IntPtr hwnd, int virtualKey)
    {
        PostMessageW(hwnd, 0x0100, new IntPtr(virtualKey), IntPtr.Zero);
        PostMessageW(hwnd, 0x0101, new IntPtr(virtualKey), new IntPtr(unchecked((int)0xC0000001)));
    }

    public static int[] CaptureClient(IntPtr hwnd, out int width, out int height)
    {
        RECT client;
        GetClientRect(hwnd, out client);
        width = client.Right;
        height = client.Bottom;
        using (var bitmap = new Bitmap(width, height, PixelFormat.Format32bppRgb))
        {
            using (var graphics = Graphics.FromImage(bitmap))
            {
                IntPtr dc = graphics.GetHdc();
                PrintWindow(hwnd, dc, PW_CLIENTONLY_RENDERFULLCONTENT);
                graphics.ReleaseHdc(dc);
            }
            return ReadRgb(bitmap);
        }
    }

    public static int[] LoadPng(string path, out int width, out int height)
    {
        using (var bitmap = new Bitmap(path))
        {
            width = bitmap.Width;
            height = bitmap.Height;
            return ReadRgb(bitmap);
        }
    }

    static int[] ReadRgb(Bitmap bitmap)
    {
        var rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        var data = bitmap.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
        var pixels = new int[bitmap.Width * bitmap.Height];
        for (int y = 0; y < bitmap.Height; ++y)
        {
            Marshal.Copy(IntPtr.Add(data.Scan0, y * data.Stride), pixels, y * bitmap.Width, bitmap.Width);
        }
        bitmap.UnlockBits(data);
        for (int i = 0; i < pixels.Length; ++i) pixels[i] &= 0x00FFFFFF;
        return pixels;
    }

    // Presenter.cpp 의 ComputeLetterboxRect 와 같은 식
    public static int[] Letterbox(long cw, long ch, long iw, long ih)
    {
        long w = cw, h = ch;
        if (cw * ih <= ch * iw) h = (cw * ih + iw / 2) / iw;
        else w = (ch * iw + ih / 2) / ih;
        long left = (cw - w) / 2, top = (ch - h) / 2;
        return new int[] { (int)left, (int)top, (int)w, (int)h };
    }

    // 한 축의 nearest 대응 범위. 배율이 정수이면 정확히 한 pixel, 아니면 반올림 차이를 허용해 앞뒤 한 pixel.
    static void SourceRange(int d, int destSize, int srcSize, out int lo, out int hi)
    {
        if (destSize % srcSize == 0) { lo = hi = d / (destSize / srcSize); return; }
        if (srcSize % destSize == 0) { int step = srcSize / destSize; lo = d * step; hi = lo + step - 1; return; }
        int center = (int)((d + 0.5) * srcSize / destSize);
        lo = Math.Max(center - 1, 0);
        hi = Math.Min(center + 1, srcSize - 1);
    }

    // 결과: { 비교한 pixel 수, 불일치 수, 두 dump 가 달라 건너뛴 pixel 수, letterbox 영역 불일치 수 }
    public static long[] Compare(int[] screen, int sw, int sh, int[] dumpA, int[] dumpB, int iw, int ih)
    {
        int[] box = Letterbox(sw, sh, iw, ih);
        long compared = 0, mismatched = 0, skipped = 0, barMismatched = 0;
        for (int y = 0; y < sh; ++y)
        {
            for (int x = 0; x < sw; ++x)
            {
                int actual = screen[y * sw + x];
                int dx = x - box[0], dy = y - box[1];
                if (dx < 0 || dy < 0 || dx >= box[2] || dy >= box[3])
                {
                    if (actual != 0) ++barMismatched;
                    continue;
                }

                int x0, x1, y0, y1;
                SourceRange(dx, box[2], iw, out x0, out x1);
                SourceRange(dy, box[3], ih, out y0, out y1);

                bool isStatic = true, matched = false;
                for (int sy = y0; sy <= y1 && isStatic; ++sy)
                {
                    for (int sx = x0; sx <= x1; ++sx)
                    {
                        int i = sy * iw + sx;
                        if (dumpA[i] != dumpB[i]) { isStatic = false; break; }
                        if (dumpA[i] == actual) matched = true;
                    }
                }

                if (!isStatic) { ++skipped; continue; }
                ++compared;
                if (!matched) ++mismatched;
            }
        }
        return new long[] { compared, mismatched, skipped, barMismatched };
    }
}
'@

function Wait-NewCapture([string[]]$Before) {
    for ($i = 0; $i -lt 50; ++$i) {
        Start-Sleep -Milliseconds 100
        $new = @(Get-ChildItem -Path captures -Recurse -Filter *.png -ErrorAction SilentlyContinue |
            Where-Object { $Before -notcontains $_.FullName })
        if ($new.Count -gt 0) { return $new[0].FullName }
    }
    throw 'F9 를 보냈지만 captures/ 에 새 PNG 가 생기지 않았다'
}

function Read-Png([string]$Path) {
    for ($i = 0; ; ++$i) {
        try {
            $w = 0; $h = 0
            $pixels = [PresentationCheck]::LoadPng($Path, [ref]$w, [ref]$h)
            return @{ Pixels = $pixels; Width = $w; Height = $h }
        }
        catch {
            if ($i -ge 20) { throw }
            Start-Sleep -Milliseconds 100
        }
    }
}

function Get-CaptureFiles {
    @(Get-ChildItem -Path captures -Recurse -Filter *.png -ErrorAction SilentlyContinue | ForEach-Object FullName)
}

function Invoke-Capture([IntPtr]$Hwnd, [System.Collections.Generic.List[string]]$Created) {
    $before = Get-CaptureFiles
    [PresentationCheck]::PressKey($Hwnd, [PresentationCheck]::VK_F9)
    $path = Wait-NewCapture $before
    $Created.Add($path)
    return $path
}

[PresentationCheck]::UsePerMonitorV2Dpi()

$process = Start-Process -FilePath $Exe -WorkingDirectory (Get-Location) -PassThru
$created = [System.Collections.Generic.List[string]]::new()
$failed = $false
try {
    while ($process.MainWindowHandle -eq 0) {
        if ($process.HasExited) { throw "SoftPipe 가 바로 종료됐다: exit code $($process.ExitCode)" }
        Start-Sleep -Milliseconds 100
        $process.Refresh()
    }
    $hwnd = $process.MainWindowHandle
    [PresentationCheck]::PressKey($hwnd, [PresentationCheck]::VK_PAUSE)

    $cases = [System.Collections.Generic.List[object]]::new()
    for ($i = 0; $i -lt $ClientSizes.Count; $i += 2) {
        $cases.Add(@{ Width = $ClientSizes[$i]; Height = $ClientSizes[$i + 1]; Minimize = $false })
    }
    $cases.Add(@{ Width = 1280; Height = 720; Minimize = $true })

    foreach ($case in $cases) {
        $placed = [PresentationCheck]::PlaceClient($hwnd, $case.Width, $case.Height)
        if ($placed[2] -eq 0) {
            'SKIP requested {0}x{1}: client {2}x{3} 가 모니터 밖으로 나간다' -f $case.Width, $case.Height, $placed[0], $placed[1]
            continue
        }
        if ($case.Minimize) {
            [PresentationCheck]::Minimize($hwnd)
            Start-Sleep -Milliseconds 500
            [PresentationCheck]::Restore($hwnd)
        }
        Start-Sleep -Milliseconds 500

        $pathA = Invoke-Capture $hwnd $created
        $sw = 0; $sh = 0
        $screen = [PresentationCheck]::CaptureClient($hwnd, [ref]$sw, [ref]$sh)
        $pathB = Invoke-Capture $hwnd $created

        $dumpA = Read-Png $pathA
        $dumpB = Read-Png $pathB
        $iw = $dumpA.Width; $ih = $dumpA.Height

        $r = [PresentationCheck]::Compare($screen, $sw, $sh, $dumpA.Pixels, $dumpB.Pixels, $iw, $ih)
        $ok = ($r[0] -gt 0) -and ($r[1] -eq 0) -and ($r[2] -eq 0) -and ($r[3] -eq 0)
        if (-not $ok) { $failed = $true }

        $label = "requested $($case.Width)x$($case.Height)"
        if ($case.Minimize) { $label += ' after minimize/restore' }
        '{0,-4} {1,-40} client {2}x{3}  png {4}x{5}  compared {6}  mismatched {7}  skipped {8}  letterbox-mismatched {9}' -f `
            ($(if ($ok) { 'PASS' } else { 'FAIL' })), $label, $sw, $sh, $iw, $ih, $r[0], $r[1], $r[2], $r[3]
    }
}
finally {
    if (-not $process.HasExited) {
        $process.CloseMainWindow() | Out-Null
        $process.WaitForExit(5000) | Out-Null
    }
    "exit code: $($process.ExitCode)"
    if ($process.ExitCode -ne 0) { $failed = $true }
    foreach ($path in $created) { Remove-Item -LiteralPath $path -ErrorAction SilentlyContinue }
}

if ($failed) { 'RESULT: FAIL'; exit 1 }
'RESULT: PASS'
