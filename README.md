## Repository Purpose

This repository is a fork of the original [iPerf3 repository](https://github.com/esnet/iperf) 
specifically adapted for use with the OpenMobileNetworkToolkit developed by Fraunhofer FOKUS. 
The primary goal of this fork is to make iPerf3 compatible with 
Android Apps by replacing `exit()` calls with `longjmp();`. 
Adapted versions of iPerf will be tagged with the format v3.XX.

#### Current adapted version is iPerf 3.15.

### How to use the Shared Library

#### 1. How to build/get libiperf
You can download the latest libiperf from [here](https://github.com/omnt/iperf/releases), or you can build it locally with the Docker file:
```bash
docker build -t android-ndk:latest . 
docker run -d --name android-ndk-container android-ndk 
docker cp -a android-ndk-container:/tmp/iperf/jniLibs ./ 
```


#### 2. Add the files to the Android App
Copy the executables to the jniLibs path, the default path is `app/src/main/jniLibs`.
For every [ABI](https://developer.android.com/ndk/guides/abis) a directory needs to be created.
It should look like this:

```
app/src/main/jniLibs/
├─ arm64-v8a/
│  ├─ libiperf3.12.so
├─ armeabi-v7a/
│  ├─ libiperf3.12.so
├─ x86/
│  ├─ libiperf3.12.so
├─ x86_64/
│  ├─ libiperf3.12.so
```

#### 3. Integrate iPerf into an android app

Load the Library
```Java
static {
    System.loadLibrary("iperf3.15");
    Log.i(TAG, "iperf.so loaded!");
}
```
Add the wrapepr function to the App
```Java
private native int iperf3Wrapper(String[] argv, String nativeLibraryDir);
```
Add the call to the App
```Java
iperf3Wrapper(cmd, getApplicationContext().getApplicationInfo().nativeLibraryDir);
```
For a working example have a look [here]()
## Acknowledgments

- android-iperf - [Github](https://github.com/KnightWhoSayNi/android-iperf)
- iPerf - [iperf.fr](https://github.com/esnet/iperf)

