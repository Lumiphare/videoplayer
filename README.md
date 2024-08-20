## Qt播放器实现

需要自己配制ffmpeg和qt环境

使用Qt开发UI界面，实现了播放、暂停、进度条拖动和精确定位功能

核心音视频处理都是FFmpeg，播放由Qt音频由Qt的Audioplayer类实现，视频则由QOpenGL实现。
通过拖动mp4文件到窗口当中播放
![img.png](img.png)