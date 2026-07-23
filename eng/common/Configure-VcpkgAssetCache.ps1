<#
.SYNOPSIS
Configures vcpkg to fetch build-time assets (e.g. the MSYS2 runtime/tools vcpkg bootstraps on
Windows) through Microsoft's internal Terrapin asset cache instead of reaching out to public
mirrors (mirror.msys2.org, ftp2.osuosl.org, us.mirrors.cicku.me, etc).

Direct connections to those public mirrors violate the CFSClean2/CFSClean3 network isolation
policies (SFI-ES4.2.4). See https://aka.ms/1espt-networkisolation for background.

This only applies when running in the dnceng/internal project, since the Terrapin retrieval tool
and its backing storage account are only available/authenticated on internal 1ES-hosted agents.
On any other agent (e.g. public CI), this script is a no-op and vcpkg falls back to its default
(public) download behavior.
#>

if ($env:SYSTEM_TEAMPROJECT -ne 'internal') {
    Write-Host "Not running in the internal project; leaving vcpkg asset sources unconfigured."
    return
}

$terrapin = (Get-Command TerrapinRetrievalTool.exe -ErrorAction SilentlyContinue).Source
if (-not $terrapin) {
    $fallbackPath = 'C:\local\Terrapin\TerrapinRetrievalTool.exe'
    if (Test-Path $fallbackPath) {
        $terrapin = $fallbackPath
    }
}

if ($terrapin) {
    # x-block-origin ensures vcpkg never falls back to the public internet if an asset is missing
    # from the cache, so a cache miss surfaces as a clear build failure rather than a silent
    # policy violation.
    $assetSources = "x-script,`"$terrapin`" -b https://vcpkg.storage.devpackages.microsoft.io/artifacts/ -a true -u Environment -p {url} -s {sha512} -d {dst};x-block-origin"
    Write-Host "##vso[task.setvariable variable=X_VCPKG_ASSET_SOURCES]$assetSources"
    Write-Host "Configured vcpkg asset cache using Terrapin at '$terrapin'."
}
else {
    Write-Host "##vso[task.logissue type=warning]TerrapinRetrievalTool.exe was not found on this agent. vcpkg will fall back to public mirrors, which may reintroduce CFS network isolation violations."
}
