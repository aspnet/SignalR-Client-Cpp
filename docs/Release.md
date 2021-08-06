# Release a new version of the library

## Create the release
* Get on the correct commit that we want to release (normally tip of main branch)
* `git tag <tag-name>` (previous release format: v0.1.0-alpha3)
* `git push origin <tag-name>`

## Fork
* For the repo at https://github.com/microsoft/vcpkg
  - If you already have a fork, make sure it is updated to the master branch
  ![image](https://user-images.githubusercontent.com/7574801/128568379-deb14ed7-1ba3-4cb7-83d2-faecf8300f21.png)

* Clone your fork locally `git clone https://github.com/<fork>/vcpkg.git`

## Modify vcpkg/ports/CONTROL
* Update the `Version`
* Add any new features we want users to be able to add/remove (msgpack, cpprestsdk, etc.)

## Modify vcpkg/ports/microsoft-signalr/portfile.cmake
* Change the `REF` to be the tag name
* Change the `SHA512`, this will be done in the step below
* Update any flags we might want to set a default for in `vcpkg_configure_cmake`

## Test the changes
* Run `./bootstrap-vcpkg.bat` to get the package installer
* Run `vcpkg install microsoft-signalr:x64-windows` to install the library
  - The first time this runs it will fail with the wrong SHA512 and give us the correct value for the SHA512, so replace that value
* To try out different features (defined in the CONTROL file) you can install with `vcpkg install microsoft-signalr[<feature name>]`, e.g. `<feature name>` -> `messagepack`
* Do a smoke test that the library works, a sample app like https://github.com/halter73/SignalR-Client-Cpp-Sample explains how to do that quickly

## Make a pull request
* Commit the modified files (should be 2 files minimum)
* After making a first commit run `./vcpkg x-add-version microsoft-signalr --overwrite-version`
  - This will update 2 more files that vcpkg needs updated
  - Commit the new changes
* Submit a PR from your fork to the https://github.com/microsoft/vcpkg repo
