from flask import Flask, request, jsonify
from datetime import datetime

app = Flask(__name__)

alerts = {}

approvals = []


@app.route("/alert", methods=["POST"])
def receive_alert():
    data = request.get_json(silent=True)
    if not data or "file_path" not in data:
        return jsonify({"error": "invalid payload"}), 400

    path = data["file_path"]
    alerts[path] = {
        "file_path": path,
        "old_hash": data.get("old_hash", "0"),
        "new_hash": data.get("new_hash", "0"),
        "reason": data.get("reason", ""),
        "detected_at": data.get("detected_at") or datetime.utcnow().isoformat() + "Z",
        "status": "pending",
    }
    return jsonify({"ok": True}), 200


@app.route("/alerts", methods=["GET"])
def get_alerts():
    return jsonify(list(alerts.values())), 200


@app.route("/approve", methods=["POST"])
def approve():
    data = request.get_json(silent=True)
    if not data or "file_path" not in data:
        return jsonify({"error": "invalid payload"}), 400

    path = data["file_path"]
    if path not in alerts:
        return jsonify({"error": "alert not found"}), 404

    alerts.pop(path)
    if path not in approvals:
        approvals.append(path)

    return jsonify({"ok": True}), 200


@app.route("/approvals", methods=["GET"])
def get_approvals():
    result = approvals.copy()
    approvals.clear()
    return jsonify(result), 200


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
