# Phase 5 - Stereo disparity implementation using OpenCL for a GPU

## Summary of goals
A simple but correct parallel implementation of the algorithm running on GPU.

## Host system details
Device info printing is implemented in [`print_device_info()`](../src/device_support.c), which reports the following:
```console
Device info dump:

Device is a GPU
Device vendor: NVIDIA Corporation
Device name: Quadro P600
Device supports dedicated local memory
Device local memory max size is 49152 bytes
Device supports up to 3 compute units with up to 1024 work groups each
Device has maximum clock frequency of 1620 MHz
Device supports constant buffers up to 65536 bytes
Device supports up to 3 work item dimensions
Device work item dimension 0 supports up to 1024 work items
Device work item dimension 1 supports up to 1024 work items
Device work item dimension 2 supports up to 64 work items
Device supports images

end of device info dump
```

## Implementation

[< Back to top](../README.md)
