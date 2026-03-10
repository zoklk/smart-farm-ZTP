#!/usr/bin/env python3
"""
Alert Web Server - Pi에서 직접 실행
Pod에서 알림 데이터를 받아 웹 페이지에 표시
"""

from flask import Flask, render_template_string, request, jsonify
from datetime import datetime
from collections import deque
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

app = Flask(__name__)

# 최근 알림 저장 (최대 50개)
alert_log = deque(maxlen=50)

# 간단한 알림 페이지
ALERT_LOG_HTML = """
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Zone {{ zone }} - Alert Log</title>
    <meta http-equiv="refresh" content="3">
    <style>
        body {
            font-family: monospace;
            background: #1e1e1e;
            color: #00ff00;
            padding: 20px;
        }
        .header {
            border-bottom: 2px solid #00ff00;
            padding-bottom: 10px;
            margin-bottom: 20px;
        }
        .alert {
            background: #2d2d2d;
            border-left: 4px solid #ff4444;
            padding: 10px;
            margin: 10px 0;
        }
        .timestamp { color: #888; }
        .command {
            color: #ffff00;
            font-weight: bold;
        }
        .normal {
            color: #00ff00;
            text-align: center;
            padding: 50px;
            font-size: 18px;
        }
    </style>
</head>
<body>
    <div class="header">
        <h2>ZONE {{ zone }} - CONTROL LOG</h2>
        <div>Last Update: {{ now }}</div>
        <div>Total Alerts: {{ alerts|length }}</div>
    </div>
    {% if alerts %}
        {% for alert in alerts %}
        <div class="alert">
            <div class="timestamp">{{ alert.timestamp }}</div>
            <div>Device: {{ alert.device }} | Sensor: {{ alert.sensor }}</div>
            <div>Value: {{ alert.value }} (Threshold: {{ alert.threshold }})</div>
            <div class="command">→ COMMAND: {{ alert.command }}</div>
        </div>
        {% endfor %}
    {% else %}
        <div class="normal">✓ All systems normal - No control actions needed</div>
    {% endif %}
</body>
</html>
"""


@app.route('/')
def index():
    """알림 로그 페이지 (최신순)"""
    zone = request.args.get('zone', '1')
    return render_template_string(
        ALERT_LOG_HTML,
        zone=zone,
        alerts=list(reversed(alert_log)),
        now=datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    )


@app.route('/api/alerts', methods=['POST'])
def receive_alert():
    """Pod에서 알림 데이터 수신"""
    try:
        alert_data = request.json

        # 타임스탬프 추가
        alert_data['timestamp'] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

        # 로그에 추가
        alert_log.append(alert_data)

        logger.info(f"Alert received: {alert_data['device']} - {alert_data['sensor']}: {alert_data['value']}")

        return jsonify({'status': 'success'}), 200

    except Exception as e:
        logger.error(f"Failed to receive alert: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 400


@app.route('/api/alerts', methods=['GET'])
def get_alerts():
    """알림 목록 조회 (API)"""
    return jsonify(list(alert_log))


@app.route('/health')
def health():
    """Health check"""
    return 'OK', 200


if __name__ == '__main__':
    logger.info("Starting Alert Web Server on port 8080")
    app.run(host='0.0.0.0', port=8080, debug=False)