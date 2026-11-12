Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script = Join-Path $PSScriptRoot '../Setup/Sign-Release.ps1'
$tokens = $null
$errors = $null
[System.Management.Automation.Language.Parser]::ParseFile(
    (Resolve-Path -LiteralPath $script).Path, [ref]$tokens, [ref]$errors) | Out-Null
if ($errors.Count -ne 0) { throw ($errors | Out-String) }

function Expect-Rejection([scriptblock] $Operation, [string] $Expected) {
    $rejected = $false
    try { & $Operation } catch {
        if ($_.Exception.Message -notmatch $Expected) { throw }
        $rejected = $true
    }
    if (-not $rejected) { throw "Expected rejection: $Expected" }
}

$directory = Join-Path ([System.IO.Path]::GetTempPath()) ('wrt-sign-test-' + [guid]::NewGuid())
[System.IO.Directory]::CreateDirectory($directory) | Out-Null
try {
    $text = Join-Path $directory 'not-a-binary.txt'
    [System.IO.File]::WriteAllText($text, 'test fixture')
    $thumbprint = '0' * 40
    Expect-Rejection { & $script -Thumbprint 'invalid' -Files $text } 'Thumbprint'
    Expect-Rejection { & $script -Thumbprint $thumbprint -Files $text -TimestampUrl 'http://example.com' } 'HTTPS'
    Expect-Rejection { & $script -Thumbprint $thumbprint -Files $directory } 'Not a release binary'
    Expect-Rejection { & $script -Thumbprint $thumbprint -Files $text } 'Not a release binary'
    Expect-Rejection { & $script -Thumbprint $thumbprint -Files (Join-Path $directory 'missing.exe') } 'missing\.exe'
    if ([System.IO.File]::ReadAllText($text) -ne 'test fixture') { throw 'Input was modified.' }
    Write-Output 'PASS signing script syntax and invalid-input rejection (no signing or certificate access)'
} finally {
    [System.IO.File]::Delete((Join-Path $directory 'not-a-binary.txt'))
    [System.IO.Directory]::Delete($directory)
}
