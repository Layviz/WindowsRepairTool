# Sign binaries BEFORE packaging them; sign the resulting MSI afterwards with
# the same thumbprint. Keys stay in the Windows certificate store (or its HSM).
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[A-Fa-f0-9]{40}$')]
    [string] $Thumbprint,

    [Parameter(Mandatory = $true)]
    [string[]] $Files,

    [switch] $MachineStore,
    [uri] $TimestampUrl = 'https://timestamp.digicert.com',
    [string] $SignTool = 'signtool.exe'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($TimestampUrl.Scheme -ne 'https') {
    throw 'The timestamp server must use HTTPS.'
}
if ($Files.Count -eq 0) { throw 'Supply at least one release artifact.' }

# Validate every input before changing any artifact.
$artifacts = foreach ($file in $Files) {
    $item = Get-Item -LiteralPath $file
    if ($item.PSIsContainer -or $item.Extension.ToLowerInvariant() -notin @('.exe', '.dll', '.msi')) {
        throw "Not a release binary or MSI: $file"
    }
    $item.FullName
}
$tool = Get-Command -Name $SignTool -CommandType Application
$store = if ($MachineStore) { 'LocalMachine' } else { 'CurrentUser' }
$certificate = Get-Item -LiteralPath "Cert:\$store\My\$Thumbprint"
if (-not $certificate.HasPrivateKey -or $certificate.NotAfter -le (Get-Date) -or
    $certificate.NotBefore -gt (Get-Date)) {
    throw 'A currently valid code-signing certificate with a private key is required.'
}
$usages = @($certificate.Extensions |
    Where-Object { $_ -is [System.Security.Cryptography.X509Certificates.X509EnhancedKeyUsageExtension] } |
    ForEach-Object { $_.EnhancedKeyUsages } | ForEach-Object { $_.Value })
if ('1.3.6.1.5.5.7.3.3' -notin $usages) {
    throw 'The certificate must permit code signing.'
}

foreach ($artifact in $artifacts) {
    if (-not $PSCmdlet.ShouldProcess($artifact, 'Sign and verify release artifact')) { continue }
    $signArguments = @('sign', '/sha1', $Thumbprint, '/fd', 'SHA256',
        '/tr', $TimestampUrl.AbsoluteUri, '/td', 'SHA256')
    if ($MachineStore) { $signArguments += '/sm' }
    & $tool.Source @signArguments $artifact
    if ($LASTEXITCODE -ne 0) { throw "Signing failed: $artifact (exit $LASTEXITCODE)" }
    & $tool.Source verify /pa /all $artifact
    if ($LASTEXITCODE -ne 0) { throw "Signature verification failed: $artifact (exit $LASTEXITCODE)" }
    $signature = Get-AuthenticodeSignature -LiteralPath $artifact
    if ($signature.Status -ne 'Valid' -or $null -eq $signature.SignerCertificate -or
        $signature.SignerCertificate.Thumbprint -ne $Thumbprint) {
        throw "The artifact does not have a trusted signature from the selected certificate: $artifact"
    }
}
