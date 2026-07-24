<#
.SYNOPSIS
Configures vcpkg to fetch build-time assets (e.g. the MSYS2 runtime/tools vcpkg bootstraps on
Windows) through Microsoft's internal Terrapin asset cache instead of reaching out to public
mirrors (mirror.msys2.org, ftp2.osuosl.org, us.mirrors.cicku.me, etc).

Direct connections to those public mirrors violate the CFSClean2/CFSClean3 network isolation
policies (SFI-ES4.2.4). See https://aka.ms/1espt-networkisolation for background.

Per the internal "Consuming C/C++ OSS inside Microsoft with vcpkg" docs, the Terrapin Retrieval
Tool (TRT) is not pre-installed on our DncEngInternalBuildPool (1es-windows-2022) image - it's
only native to the vcpkg SDK NuGet and OneBranch containers. Self-restoring the TRT NuGet package
ourselves would require a service connection/credentials for the msazure/One organization's
"TerrapinRetrievalTool-Feed", which our dnceng/internal pipeline does not have.

Since we only ever need read access to assets that are already mirrored (we never upload new
content to Terrapin), we use the docs' documented tool-free, no-auth fallback for read-only
consumption: pointing vcpkg directly at Terrapin's backing storage account via x-azurl. This is
the same fallback the docs recommend for CloudBuild, which similarly can't authenticate TRT.

If TRT does happen to be available on the agent (e.g. if a future pool image bakes it in
natively), prefer it via x-script since it supports additional features (e.g. just-in-time
mirroring of assets not yet cached).

This only applies when running in the dnceng/internal project, since Terrapin's storage account
is only reachable/authorized from internal 1ES-hosted agents. On any other agent (e.g. public
CI), this script is a no-op and vcpkg falls back to its default (public) download behavior.
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
# from the cache, so a cache miss surfaces as a clear build failure rather than a silent policy
# violation.
if ($terrapin) {
    $assetSources = "x-script,`"$terrapin`" -b https://vcpkg.storage.devpackages.microsoft.io/artifacts/ -a true -u Environment -p {url} -s {sha512} -d {dst};x-block-origin"
    Write-Host "##vso[task.setvariable variable=X_VCPKG_ASSET_SOURCES]$assetSources"
    Write-Host "Configured vcpkg asset cache using Terrapin (TRT) at '$terrapin'."
}
else {
    $assetSources = "x-azurl,https://vcpkg.storage.devpackages.microsoft.io/artifacts/;x-block-origin"
    Write-Host "##vso[task.setvariable variable=X_VCPKG_ASSET_SOURCES]$assetSources"
    Write-Host "TerrapinRetrievalTool.exe was not found on this agent. Configured vcpkg asset cache using Terrapin's storage account directly (read-only, no tool required)."
}
