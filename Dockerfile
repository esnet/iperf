FROM ubuntu:22.04
LABEL maintainer="johann.hackler@fokus.fraunhofer.de"

# Update and install dependencies
RUN apt-get -y update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    tzdata make bash git unzip wget curl openjdk-8-jdk build-essential autoconf nano tree python3 libpthread-stubs0-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ARG API_VERSION=31
ARG SDK_VERSION=4333796
ARG NDK_VERSION=r26b
ARG IPERF_VERSION=iperf-3.16

ENV ANDROID_SDK_VERSION=${SDK_VERSION}
ENV ANDROID_SDK_HOME=/opt/android-sdk
ENV ANDROID_SDK_FILENAME=sdk-tools-linux-${ANDROID_SDK_VERSION}
ENV ANDROID_SDK_URL=https://dl.google.com/android/repository/${ANDROID_SDK_FILENAME}.zip

# Download and install Android SDK
RUN wget --no-check-certificate -q ${ANDROID_SDK_URL} && \
    mkdir -p ${ANDROID_SDK_HOME} && \
    unzip -q ${ANDROID_SDK_FILENAME}.zip -d ${ANDROID_SDK_HOME} && \
    rm -f ${ANDROID_SDK_FILENAME}.zip

ENV PATH=${PATH}:${ANDROID_SDK_HOME}/tools:${ANDROID_SDK_HOME}/tools/bin:${ANDROID_SDK_HOME}/platform-tools
ENV PATH=${PATH}:/home/nespl/android-ndk-r8

# Accept Android SDK licenses and install required components
RUN yes | sdkmanager --licenses > /dev/null && \
    yes | sdkmanager "platforms;android-${API_VERSION}" > /dev/null

ENV ANDROID_NDK_VERSION=${NDK_VERSION}
ENV ANDROID_NDK_HOME=/opt/android-ndk
ENV ANDROID_NDK_FILENAME=android-ndk-${ANDROID_NDK_VERSION}-linux
ENV ANDROID_NDK_URL=https://dl.google.com/android/repository/${ANDROID_NDK_FILENAME}.zip

# Download and install Android NDK
RUN wget --no-check-certificate -q ${ANDROID_NDK_URL} && \
    mkdir -p ${ANDROID_NDK_HOME} && \
    unzip -q ${ANDROID_NDK_FILENAME}.zip -d ${ANDROID_NDK_HOME} && \
    rm -f ${ANDROID_NDK_FILENAME}.zip

ENV PATH=${PATH}:${ANDROID_NDK_HOME}/android-ndk-${NDK_VERSION}/
ENV NDK_PROJECT_PATH=/tmp

COPY . /tmp/iperf
RUN cd /tmp/iperf && ./configure

RUN mkdir -p /tmp/iperf/jniLibs
RUN ndk-build APP_BUILD_SCRIPT=/tmp/iperf/jni/Android.mk NDK_LIBS_OUT=/tmp/iperf/jniLibs
