# Code provided by Wicky
from flask import Flask, Response
import cv2

app = Flask(__name__)

IMAGE_FILE = "./chungus.jpg"

def generate():
    while True:
        frame = cv2.imread(IMAGE_FILE)

        if frame is None:
            continue

        frame = cv2.resize(frame, (240, 240))

        ret, jpeg = cv2.imencode(".jpg", frame, [cv2.IMWRITE_JPEG_QUALITY, 30])
        if not ret:
            continue

        jpg_bytes = jpeg.tobytes()

        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n'
               b'Content-Length: ' + str(len(jpg_bytes)).encode() + b'\r\n\r\n' +
               jpg_bytes + b'\r\n')

@app.route("/stream")
def stream():
    return Response(generate(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, threaded=True)
