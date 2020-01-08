# Docker Windows Patcher
> A memory patch to work around Windows bug with Docker

## Motivation
This project aims to work around the following blocking bug in Docker for Windows in process isolation mode, whenever image building is attempted:
```
Error response from daemon: hcsshim::PrepareLayer failed in Win32: Incorrect function. (0x1)
```

## Explanation
This patcher injects its DLL into `dockerd.exe`. The DLL will then patch the loaded module `vmcompute.dll`, attempting to work around the Windows bug. Quote from source:
> After debugging for a few hours I stumbled upon a call to `AttachVirtualDisk` which is used to mount the images.
> Some undocumented flags were passed to the function during one of the calls and I found out that removing the undocumented flags resolves the issue for some reason.
> I have created a simple program to patch the running dockerd daemon in memory and remove the undocumented flags.

More information on the problem, workaround and implementation can be found on [the issue discussion](https://github.com/microsoft/hcsshim/issues/624#issuecomment-509526835)

This project takes the above idea and improves upon it by providing a more robust, update-agnostic solution

## Note
While the program works perfectly, it did not solve my problem but instead introduced error `0x57`, which appears to be related to [another Windows bug](https://github.com/microsoft/hcsshim/issues/708)