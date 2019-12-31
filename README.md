# WatchTogether

WatchTogether is an application to watch videos together with a friend. Simply open the file you want to watch on your computer, tell your friend your IP address, and they will be able to connect and watch the video with you. All controls will be synchronized between you, so if your friend needs to grab some snacks, they can pause the video on both ends and resume playback later. 

## Download

Currently, the application is only for Win64. 

[Windows 64-bit](https://github.com/EverCursed/watch-together/releases/tag/0.0.1)

## Usage

### Host (computer with the video file)

You will need to select the video which to play. Just run the `host.bat` file and enter the full file path of the video file to start hosting. Alternatively you can run the main application as follows:

```
watchtogether.exe -i <file path> -s
```

### Client (computer connecting to the host)

You can run the `connect.bat` file and enter the host's IP address. Alternatively, you can run the main application as follows:

```
watchtogether.exe -p <ip address>
```
