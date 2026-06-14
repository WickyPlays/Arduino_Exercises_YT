# Code provided by Wicky
from flask import Flask, Response
import yt_dlp
import cv2
import time

app = Flask(__name__)

youtube_url = "https://www.twitch.tv/caedrel"

ydl_opts = {
    "format": "best[ext=mp4]/best",
    "quiet": True,
}

def generate_frames():

    with yt_dlp.YoutubeDL(ydl_opts) as ydl:
        info = ydl.extract_info(youtube_url, download=False)
        stream_url = info["url"]

    cap = cv2.VideoCapture(stream_url)

    fps = cap.get(cv2.CAP_PROP_FPS)

    if fps <= 0:
        fps = 30

    fps = min(fps, 30)

    frame_delay = 1.0 / fps

    while True:

        ret, frame = cap.read()

        if not ret:
            cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
            continue

        frame = cv2.resize(frame, (240, 240))

        success, jpeg = cv2.imencode(
            ".jpg",
            frame,
            [cv2.IMWRITE_JPEG_QUALITY, 40]
        )

        if not success:
            continue

        jpg = jpeg.tobytes()

        yield (
            b'--frame\r\n'
            b'Content-Type: image/jpeg\r\n'
            b'Content-Length: ' +
            str(len(jpg)).encode() +
            b'\r\n\r\n' +
            jpg +
            b'\r\n'
        )

        time.sleep(frame_delay)


@app.route("/stream")
def stream():

    return Response(
        generate_frames(),
        mimetype='multipart/x-mixed-replace; boundary=frame'
    )


if __name__ == "__main__":

    app.run(
        host="0.0.0.0",
        port=5000,
        threaded=True
    )
