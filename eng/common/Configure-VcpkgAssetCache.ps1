<#
.SYNOPSIS
Configures vcpkg to fetch build-time assets (e.g. the MSYS2 runtime/tools vcpkg bootstraps on
Windows) through Microsoft's internal Terrapin asset cache instead of reaching out to public
mirrors (mirror.msys2.org, ftp2.osuosl.org, us.mirrors.cicku.me, etc), which trip CFS network
isolation policy violations.

Our build pool doesn't have the Terrapin tool installed, and restoring it ourselves would need
a service connection we don't have. Since we only ever need read access (never uploading new
assets), we fall back to pointing vcpkg directly at Terrapin's storage endpoint via x-azurl - no
extra tooling or auth required. If the tool is ever found on the agent, prefer it instead.

This only applies in the dnceng/internal project; on any other agent (e.g. public CI), this
script is a no-op and vcpkg falls back to its default (public) download behavior.
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

# x-block-origin ensures vcpkg never falls back to the public internet if an asset is missing
# from the cache, so a cache miss surfaces as a build failure rather than a policy violation.
if ($terrapin) {
    $assetSources = "x-script,`"$terrapin`" -b https://vcpkg.storage.devpackages.microsoft.io/artifacts/ -a true -u Environment -p {url} -s {sha512} -d {dst};x-block-origin"
    Write-Host "##vso[task.setvariable variable=X_VCPKG_ASSET_SOURCES]$assetSources"
    Write-Host "Configured vcpkg asset cache using Terrapin (TRT) at '$terrapin'."
}
else {
    $assetSources = "x-azurl,https://vcpkg.storage.devpackages.microsoft.io/artifacts/;x-block-origin"
    Write-Host "##vso[task.setvariable variable=X_VCPKG_ASSET_SOURCES]$assetSources"
    Write-Host "TerrapinRetrievalTool.exe was not found on this agent. Configured vcpkg asset cache using Terrapin's storage endpoint directly (read-only, no tool required)."
}
