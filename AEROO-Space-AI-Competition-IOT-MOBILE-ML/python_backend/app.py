from flask import Flask, jsonify, request
from flask_cors import CORS
import pickle
import numpy as np
import random
import requests
import os
import json
import datetime
from pathlib import Path
from dotenv import load_dotenv

# Google Drive API imports
try:
    from google.oauth2.credentials import Credentials
    from google_auth_oauthlib.flow import InstalledAppFlow
    from google.auth.transport.requests import Request
    from googleapiclient.discovery import build
    from googleapiclient.http import MediaFileUpload
    import pickle as gpickle
    GOOGLE_DRIVE_AVAILABLE = True
except ImportError:
    print("⚠️  Google Drive API not available. Run: pip install google-api-python-client google-auth-httplib2 google-auth-oauthlib")
    GOOGLE_DRIVE_AVAILABLE = False

# Load environment variables from .env file
load_dotenv()

app = Flask(__name__)
CORS(app)  # CORS'u etkinleştir (web sayfasından istek alabilmek için)

# Model ve scaler'ı yükle
try:
    with open('battery_model.pkl', 'rb') as f:
        model = pickle.load(f)

    with open('scaler.pkl', 'rb') as f:
        scaler = pickle.load(f)

    print("Model başarıyla yüklendi!")
except FileNotFoundError:
    print("HATA: Model dosyaları bulunamadı. Önce train_model.py çalıştırın!")
    model = None
    scaler = None

# Simülasyon için değişkenler
cycle_count = 0
capacity_fade = 0.0

# Vision data dosyasını sırayla okumak için
vision_line_index = 0
vision_lines_cache = []

# OpenAI API Key (from environment variable)
OPENAI_API_KEY = os.getenv('OPENAI_API_KEY', '')  # Set via environment variable or .env file

# ================================
# LOGGING & GOOGLE DRIVE CONFIGURATION
# ================================

# Google Drive API Scopes
SCOPES = ['https://www.googleapis.com/auth/drive.file']

# Logging directories
LOGS_DIR = Path('logs')
CRITICAL_LOGS_DIR = LOGS_DIR / 'critical_events'
HOURLY_LOGS_DIR = LOGS_DIR / 'hourly_snapshots'

# Create directories if they don't exist
LOGS_DIR.mkdir(exist_ok=True)
CRITICAL_LOGS_DIR.mkdir(exist_ok=True)
HOURLY_LOGS_DIR.mkdir(exist_ok=True)

# Google Drive folder ID (will be created on first upload)
DRIVE_FOLDER_ID = None

# Logging thresholds
CRITICAL_BATTERY_THRESHOLD = 15  # Log when battery < 15% (truly critical!)
CRITICAL_PANEL_THRESHOLD = 30    # Log when panel < 30% (serious issues)

@app.route('/')
def home():
    return jsonify({
        'status': 'running',
        'message': 'Battery Health Prediction API',
        'endpoints': {
            '/predict': 'POST - Batarya sağlığı tahmini',
            '/status': 'GET - API durumu'
        }
    })

@app.route('/status')
def status():
    return jsonify({
        'status': 'ok',
        'model_loaded': model is not None,
        'cycle_count': cycle_count,
        'capacity_fade': capacity_fade
    })

@app.route('/predict', methods=['POST', 'GET'])
def predict():
    global cycle_count, capacity_fade

    if model is None:
        return jsonify({'error': 'Model yüklenmedi'}), 500

    try:
        # Sıcaklık verisini al (kart veya query parametresinden)
        if request.method == 'POST':
            data = request.get_json()
            temperature = data.get('temperature', 25.0)
        else:
            temperature = float(request.args.get('temperature', 25.0))

        # Diğer verileri random üret (gerçek uygulamada sensörlerden gelecek)
        voltage = random.uniform(3.6, 4.1)  # Tipik Li-ion voltaj aralığı
        current = random.uniform(0.5, 1.5)  # Akım (A)

        # Simülasyon: Her çağrıda biraz yaşlan
        cycle_count += 1
        # Hızlı degradasyon (5 dakikada 100% → 0%)
        # 5 dk = 300 sn, 10 sn interval = 30 cycle
        # 100% / 30 cycle = ~3.3% per cycle
        capacity_fade += random.uniform(3.0, 3.5)

        # Capacity fade'i sınırla
        capacity_fade = min(capacity_fade, 100.0)  # Maksimum %100 degradasyon

        # Model için feature'ları hazırla
        features = np.array([[
            temperature,
            voltage,
            current,
            cycle_count,
            capacity_fade
        ]])

        # Normalizasyon
        features_scaled = scaler.transform(features)

        # Tahmin
        rul_percent = model.predict(features_scaled)[0]

        # Tahmin edilen kalan kullanım süresi (basit hesaplama)
        # Ortalama batarya ömrü 500-1000 döngü olduğunu varsayalım
        estimated_total_cycles = 800
        remaining_cycles = int((rul_percent / 100) * estimated_total_cycles)

        # Health status
        if rul_percent > 80:
            health_status = "Excellent"
            health_color = "green"
        elif rul_percent > 60:
            health_status = "Good"
            health_color = "lightgreen"
        elif rul_percent > 40:
            health_status = "Fair"
            health_color = "orange"
        elif rul_percent > 20:
            health_status = "Poor"
            health_color = "darkorange"
        else:
            health_status = "Critical"
            health_color = "red"

        # JSON response
        response = {
            'success': True,
            'measurements': {
                'temperature': round(temperature, 2),
                'voltage': round(voltage, 2),
                'current': round(current, 2),
                'cycle_count': cycle_count,
                'capacity_fade': round(capacity_fade, 2)
            },
            'prediction': {
                'battery_health_percent': round(rul_percent, 1),
                'health_status': health_status,
                'health_color': health_color,
                'remaining_cycles': remaining_cycles,
                'estimated_total_cycles': estimated_total_cycles
            }
        }

        return jsonify(response)

    except Exception as e:
        return jsonify({'error': str(e)}), 400

@app.route('/reset', methods=['POST'])
def reset():
    """Reset simulation"""
    global cycle_count, capacity_fade
    cycle_count = 0
    capacity_fade = 0.0
    return jsonify({'message': 'Simulation reset successfully'})

@app.route('/vision', methods=['GET'])
def vision():
    """Read vision data sequentially from file (auto-load new data when added)"""
    import json
    import os
    global vision_line_index, vision_lines_cache

    vision_file = r'C:\Users\Suleyman\Desktop\STM32 training\STM32N6-GettingStarted-ObjectDetection-main\vision_data.txt'

    try:
        if not os.path.exists(vision_file):
            return jsonify({'error': 'Vision data file not found', 'data': None}), 404

        # Load all lines into cache if empty
        if not vision_lines_cache:
            with open(vision_file, 'r') as f:
                vision_lines_cache = [line.strip() for line in f.readlines() if line.strip()]

            if not vision_lines_cache:
                return jsonify({'error': 'No vision data available', 'data': None}), 404

            vision_line_index = 0
            print(f"Vision cache loaded: {len(vision_lines_cache)} lines")
        else:
            # Check if new lines were added to the file
            with open(vision_file, 'r') as f:
                current_file_lines = [line.strip() for line in f.readlines() if line.strip()]

            # If file has more lines than cache, append new lines
            if len(current_file_lines) > len(vision_lines_cache):
                new_lines = current_file_lines[len(vision_lines_cache):]
                vision_lines_cache.extend(new_lines)
                print(f"Vision cache updated: +{len(new_lines)} new lines (total: {len(vision_lines_cache)})")

        # Get current line
        current_line = vision_lines_cache[vision_line_index]
        vision_data = json.loads(current_line)

        # Move to next line (stop at end, don't loop)
        if vision_line_index < len(vision_lines_cache) - 1:
            vision_line_index = vision_line_index + 1

        # Calculate panel health score
        # Class mapping: 0=cover, 1=crack, 2=dust, 3=normal
        cover_count = 0
        crack_count = 0
        dust_count = 0
        normal_count = 0
        total_confidence = 0

        for obj in vision_data.get('objs', []):
            label = obj.get('label', '')
            c = obj.get('c', -1)
            p = obj.get('p', 0)

            if label == 'cover' or c == 0:
                cover_count += 1
                total_confidence += p
            elif label == 'crack' or c == 1:
                crack_count += 1
                total_confidence += p
            elif label == 'dust' or c == 2:
                dust_count += 1
                total_confidence += p
            elif label == 'normal' or c == 3:
                normal_count += 1
            else:
                normal_count += 1  # Unknown objects treated as normal

        # Health score: 100 - (dust_weight * dust_count + cover_weight * cover_count + crack_weight * crack_count)
        health_score = max(0, 100 - (dust_count * 15 + cover_count * 25 + crack_count * 20))

        # Panel status
        if health_score > 80:
            panel_status = 'Excellent'
            panel_color = 'green'
        elif health_score > 60:
            panel_status = 'Good'
            panel_color = 'lightgreen'
        elif health_score > 40:
            panel_status = 'Fair'
            panel_color = 'orange'
        elif health_score > 20:
            panel_status = 'Poor'
            panel_color = 'darkorange'
        else:
            panel_status = 'Critical'
            panel_color = 'red'

        return jsonify({
            'success': True,
            'raw_data': vision_data,
            'analysis': {
                'cover_count': cover_count,
                'crack_count': crack_count,
                'dust_count': dust_count,
                'normal_count': normal_count,
                'total_objects': cover_count + crack_count + dust_count + normal_count,
                'inference_ms': vision_data.get('inf_ms', 0),
                'panel_health_score': health_score,
                'panel_status': panel_status,
                'panel_color': panel_color,
                'current_index': vision_line_index,
                'total_lines': len(vision_lines_cache)
            }
        })

    except json.JSONDecodeError as e:
        return jsonify({'error': f'Invalid JSON: {str(e)}', 'data': None}), 400
    except Exception as e:
        return jsonify({'error': str(e), 'data': None}), 500

# ================================
# GOOGLE DRIVE & LOGGING FUNCTIONS
# ================================

def get_google_drive_service():
    """Authenticate and return Google Drive service"""
    if not GOOGLE_DRIVE_AVAILABLE:
        return None

    creds = None
    token_path = 'token.pickle'
    credentials_path = 'credentials.json'

    # Check if credentials.json exists
    if not os.path.exists(credentials_path):
        print("⚠️  credentials.json not found. Google Drive upload disabled.")
        print("    Follow setup guide in README to enable Google Drive.")
        return None

    # Load saved credentials
    if os.path.exists(token_path):
        with open(token_path, 'rb') as token:
            creds = gpickle.load(token)

    # If no valid credentials, login
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            try:
                flow = InstalledAppFlow.from_client_secrets_file(credentials_path, SCOPES)
                creds = flow.run_local_server(port=0)
            except Exception as e:
                print(f"⚠️  Google Drive authentication failed: {e}")
                return None

        # Save credentials
        with open(token_path, 'wb') as token:
            gpickle.dump(creds, token)

    try:
        service = build('drive', 'v3', credentials=creds)
        return service
    except Exception as e:
        print(f"⚠️  Failed to build Drive service: {e}")
        return None


def get_or_create_drive_folder(service, folder_name='AEROO_Solar_Logs'):
    """Get or create Google Drive folder for logs"""
    global DRIVE_FOLDER_ID

    if DRIVE_FOLDER_ID:
        return DRIVE_FOLDER_ID

    try:
        # Search for existing folder
        query = f"name='{folder_name}' and mimeType='application/vnd.google-apps.folder' and trashed=false"
        results = service.files().list(q=query, spaces='drive', fields='files(id, name)').execute()
        folders = results.get('files', [])

        if folders:
            DRIVE_FOLDER_ID = folders[0]['id']
            print(f"✅ Using existing Drive folder: {folder_name} (ID: {DRIVE_FOLDER_ID})")
        else:
            # Create new folder
            folder_metadata = {
                'name': folder_name,
                'mimeType': 'application/vnd.google-apps.folder'
            }
            folder = service.files().create(body=folder_metadata, fields='id').execute()
            DRIVE_FOLDER_ID = folder.get('id')
            print(f"✅ Created new Drive folder: {folder_name} (ID: {DRIVE_FOLDER_ID})")

        return DRIVE_FOLDER_ID
    except Exception as e:
        print(f"⚠️  Failed to create/get Drive folder: {e}")
        return None


def upload_to_google_drive(file_path, folder_id=None):
    """Upload file to Google Drive"""
    if not GOOGLE_DRIVE_AVAILABLE:
        print("⚠️  Google Drive upload skipped (API not available)")
        return False

    service = get_google_drive_service()
    if not service:
        print("⚠️  Google Drive upload skipped (authentication failed)")
        return False

    try:
        if not folder_id:
            folder_id = get_or_create_drive_folder(service)

        file_metadata = {
            'name': os.path.basename(file_path),
            'parents': [folder_id] if folder_id else []
        }

        media = MediaFileUpload(file_path, resumable=True)
        file = service.files().create(
            body=file_metadata,
            media_body=media,
            fields='id, webViewLink'
        ).execute()

        print(f"✅ Uploaded to Drive: {file_metadata['name']} (ID: {file.get('id')})")
        print(f"   View at: {file.get('webViewLink')}")
        return True
    except Exception as e:
        print(f"⚠️  Drive upload failed: {e}")
        return False


def log_to_file(data, log_type='critical'):
    """Save log data to local JSON file"""
    timestamp = datetime.datetime.now()
    date_str = timestamp.strftime('%Y-%m-%d')
    time_str = timestamp.strftime('%H-%M-%S')

    # Choose directory
    if log_type == 'critical':
        log_dir = CRITICAL_LOGS_DIR
        filename = f"critical_event_{date_str}_{time_str}.json"
    else:  # hourly
        log_dir = HOURLY_LOGS_DIR
        filename = f"hourly_snapshot_{date_str}_{time_str}.json"

    file_path = log_dir / filename

    # Add timestamp to data
    data['logged_at'] = timestamp.isoformat()

    # Write to file
    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
        print(f"✅ Logged to file: {file_path}")
        return str(file_path)
    except Exception as e:
        print(f"⚠️  Failed to write log file: {e}")
        return None


def should_log_critical(battery_health, panel_health):
    """Determine if event is critical and should be logged"""
    if battery_health < CRITICAL_BATTERY_THRESHOLD:
        return True, f"Battery critical: {battery_health:.1f}%"

    if panel_health < CRITICAL_PANEL_THRESHOLD:
        return True, f"Panel critical: {panel_health:.1f}%"

    return False, None


def call_openai_for_alert(user_question, battery_health, panel_health, temperature, voltage, current):
    """Call OpenAI API for quick alert generation"""
    if not OPENAI_API_KEY:
        return {'response': 'AI analysis unavailable (API key not configured)'}

    try:
        system_prompt = f"""You are a concise AI assistant for solar battery monitoring. Generate SHORT critical alerts (1-2 sentences max).

CURRENT STATUS:
- Battery Health: {battery_health}%
- Panel Health: {panel_health}%
- Temperature: {temperature}°C
- Voltage: {voltage}V
- Current: {current}A

Provide a brief, actionable alert. Be direct and specific."""

        headers = {
            'Authorization': f'Bearer {OPENAI_API_KEY}',
            'Content-Type': 'application/json'
        }

        payload = {
            "model": "gpt-4o-mini",
            "messages": [
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": user_question}
            ],
            "temperature": 0.7,
            "max_tokens": 100
        }

        api_response = requests.post(
            'https://api.openai.com/v1/chat/completions',
            headers=headers,
            json=payload,
            timeout=10
        )

        if api_response.status_code == 200:
            response_text = api_response.json()['choices'][0]['message']['content']
            return {'response': response_text}
        else:
            return {'response': f'System alert: Battery {battery_health}%, Panel {panel_health}%'}
    except Exception as e:
        print(f"⚠️  OpenAI alert error: {e}")
        return {'response': f'System alert: Battery {battery_health}%, Panel {panel_health}%'}


# ================================
# LOGGING API ENDPOINTS
# ================================

@app.route('/log-critical-event', methods=['POST'])
def log_critical_event():
    """Log critical events to file and Google Drive with AI analysis"""
    try:
        data = request.get_json()

        # Validate data
        if not data:
            return jsonify({'error': 'No data provided'}), 400

        # Add event type
        data['event_type'] = 'CRITICAL_EVENT'
        data['timestamp'] = datetime.datetime.now().isoformat()

        # Get AI analysis if not already present
        if 'ai_alert' not in data or not data['ai_alert']:
            try:
                battery = data.get('battery', {})
                panel = data.get('panel', {})

                battery_health = battery.get('health', 0)
                panel_health = panel.get('health', 'N/A')
                temperature = battery.get('temperature', 0)
                voltage = battery.get('voltage', 0)
                current = battery.get('current', 0)

                # Prepare AI context
                ai_question = f"Quick alert: Battery at {battery_health}%, Panel {panel_health}%, Temp {temperature}°C. What's critical?"

                # Call AI assistant (short version)
                ai_response = call_openai_for_alert(
                    user_question=ai_question,
                    battery_health=battery_health,
                    panel_health=panel_health,
                    temperature=temperature,
                    voltage=voltage,
                    current=current
                )

                # Extract short summary (first 2 sentences)
                full_alert = ai_response.get('response', 'System requires attention')
                sentences = full_alert.split('.')
                short_summary = '. '.join(sentences[:2]).strip()
                if short_summary and not short_summary.endswith('.'):
                    short_summary += '.'

                # Add both to data
                data['ai_alert'] = full_alert
                data['ai_summary'] = short_summary

                print(f"✨ AI Summary: {short_summary}")
            except Exception as e:
                print(f"⚠️  AI analysis error: {e}")
                data['ai_summary'] = "Critical event detected"

        # Log to local file
        file_path = log_to_file(data, log_type='critical')

        if not file_path:
            return jsonify({'error': 'Failed to create log file'}), 500

        # Try to upload to Google Drive
        upload_success = False
        if GOOGLE_DRIVE_AVAILABLE:
            upload_success = upload_to_google_drive(file_path)

        return jsonify({
            'success': True,
            'message': 'Critical event logged successfully',
            'file_path': file_path,
            'uploaded_to_drive': upload_success,
            'timestamp': data['timestamp'],
            'ai_summary': data.get('ai_summary', '')
        })

    except Exception as e:
        print(f"⚠️  Log critical event error: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/log-hourly-snapshot', methods=['POST'])
def log_hourly_snapshot():
    """Log hourly system snapshot to file and Google Drive"""
    try:
        data = request.get_json()

        # Validate data
        if not data:
            return jsonify({'error': 'No data provided'}), 400

        # Add snapshot type
        data['snapshot_type'] = 'HOURLY_SNAPSHOT'
        data['timestamp'] = datetime.datetime.now().isoformat()

        # Log to local file
        file_path = log_to_file(data, log_type='hourly')

        if not file_path:
            return jsonify({'error': 'Failed to create log file'}), 500

        # Try to upload to Google Drive
        upload_success = False
        if GOOGLE_DRIVE_AVAILABLE:
            upload_success = upload_to_google_drive(file_path)

        return jsonify({
            'success': True,
            'message': 'Hourly snapshot logged successfully',
            'file_path': file_path,
            'uploaded_to_drive': upload_success,
            'timestamp': data['timestamp']
        })

    except Exception as e:
        print(f"⚠️  Log hourly snapshot error: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/ai-assistant', methods=['POST'])
def ai_assistant():
    """AI Assistant for solar panel and battery health analysis"""
    try:
        data = request.get_json()
        user_question = data.get('question', '')

        if not user_question:
            return jsonify({'error': 'No question provided'}), 400

        # Get current system status
        battery_status = {
            'cycle_count': cycle_count,
            'capacity_fade': round(capacity_fade, 2),
            'health_estimate': max(0, 100 - capacity_fade)
        }

        # Get current vision status
        vision_status = {'status': 'unavailable'}
        try:
            if vision_lines_cache and vision_line_index < len(vision_lines_cache):
                import json as json_module
                current_vision = json_module.loads(vision_lines_cache[max(0, vision_line_index - 1)])
                cover_count = sum(1 for obj in current_vision.get('objs', []) if obj.get('label') == 'cover' or obj.get('c') == 0)
                crack_count = sum(1 for obj in current_vision.get('objs', []) if obj.get('label') == 'crack' or obj.get('c') == 1)
                dust_count = sum(1 for obj in current_vision.get('objs', []) if obj.get('label') == 'dust' or obj.get('c') == 2)
                normal_count = sum(1 for obj in current_vision.get('objs', []) if obj.get('label') == 'normal' or obj.get('c') == 3)
                panel_health = max(0, 100 - (dust_count * 15 + cover_count * 25 + crack_count * 20))

                vision_status = {
                    'cover_count': cover_count,
                    'crack_count': crack_count,
                    'dust_count': dust_count,
                    'normal_count': normal_count,
                    'panel_health': panel_health,
                    'inference_ms': current_vision.get('inf_ms', 0)
                }
        except:
            pass

        # Build context for AI
        battery_health = battery_status['health_estimate']
        panel_health = vision_status.get('panel_health', 'N/A')

        # Determine battery condition
        if battery_health > 80:
            battery_condition = "Excellent condition"
        elif battery_health > 60:
            battery_condition = "Good condition, normal wear"
        elif battery_health > 40:
            battery_condition = "Fair condition, showing degradation"
        elif battery_health > 20:
            battery_condition = "Poor condition, needs monitoring"
        else:
            battery_condition = "Critical condition, replacement recommended soon"

        # Determine panel condition
        if panel_health != 'N/A':
            if panel_health > 80:
                panel_condition = "Excellent, no issues detected"
            elif panel_health > 60:
                panel_condition = "Good, minor issues present"
            elif panel_health > 40:
                panel_condition = "Fair, cleaning or maintenance recommended"
            elif panel_health > 20:
                panel_condition = "Poor, immediate attention needed"
            else:
                panel_condition = "Critical, severe issues detected"
        else:
            panel_condition = "No data available"

        system_prompt = f"""You are an expert AI assistant for STM32-based solar battery monitoring system (AEROO Competition Project).

REAL-TIME SYSTEM STATUS (Use these exact values in your responses):
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🔋 BATTERY STATUS:
   • Health: {battery_health:.1f}% ({battery_condition})
   • Cycle Count: {battery_status['cycle_count']} cycles
   • Capacity Fade: {battery_status['capacity_fade']:.1f}%

☀️ SOLAR PANEL STATUS:
   • Panel Health: {panel_health}% ({panel_condition})
   • Coverage Issues: {vision_status.get('cover_count', 0)} detected
   • Cracks: {vision_status.get('crack_count', 0)} detected
   • Dust/Dirt: {vision_status.get('dust_count', 0)} areas
   • Normal Areas: {vision_status.get('normal_count', 0)} detected
   • AI Inference: {vision_status.get('inference_ms', 'N/A')} ms
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

INSTRUCTIONS:
1. ALWAYS use the exact numbers from the system status above
2. If battery health < 50% or panel health < 50%, IMMEDIATELY warn the user
3. Provide specific, creative recommendations based on ACTUAL detected issues:
   - Dust detected: "Clean panels at dawn/dusk when temperatures are cooler. Use soft brush and deionized water."
   - Cracks: "Inspect crack depth. Micro-cracks (<1mm) may be monitored. Larger cracks risk water ingress and require sealing."
   - Coverage: "Check for fallen leaves, bird droppings, or snow accumulation. Consider installing mesh guards."
   - Low battery: "Reduce charge/discharge cycles. Check for parasitic loads. Consider load balancing."
   - Temperature extremes: "Insulate battery compartment. Ensure adequate ventilation. Check thermal management."
4. Be insightful and educational - explain WHY something matters:
   - "Dust reduces efficiency by 5-10% per layer. Regular cleaning maintains peak performance."
   - "Battery health below 80% means capacity loss. Each 10% drop reduces runtime proportionally."
5. Provide ACTIONABLE steps, not just warnings:
   - Instead of "Battery is degrading", say "Battery at {battery_health}% - Schedule replacement before 50% to avoid sudden failure."
6. Use analogies for complex concepts:
   - "Think of battery health like phone battery - below 50%, you start noticing shorter runtime."
7. Be concise but complete (2-4 sentences)
8. For creative questions, suggest innovative solutions:
   - AI-powered cleaning schedules based on weather
   - Predictive maintenance using degradation trends
   - Energy optimization strategies

CRITICAL ALERTS (Priority Override):
{f"⚠️ BATTERY CRITICAL: Only {battery_health:.1f}% health remaining!" if battery_health < 30 else ""}
{f"⚠️ PANEL CRITICAL: Only {panel_health}% health!" if panel_health != 'N/A' and panel_health < 30 else ""}

EXAMPLES OF GOOD RESPONSES:
Q: "Should I clean the solar panels?"
A: "Yes! {vision_status.get('dust_count', 0)} dust areas detected. Clean at dawn with soft brush and deionized water. Dust reduces efficiency by 5-10% per layer. Your panel is at {panel_health}% - cleaning could recover lost capacity."

Q: "What's causing battery degradation?"
A: "Battery at {battery_health:.1f}% after {battery_status['cycle_count']} cycles. Normal Li-ion batteries lose ~0.1% per cycle. You're seeing {battery_status['capacity_fade']:.1f}% fade. Consider temperature and charge rate factors."

Answer based on REAL DATA, not generic advice. Be helpful, educational, and specific."""

        # Call OpenAI API via requests
        headers = {
            'Authorization': f'Bearer {OPENAI_API_KEY}',
            'Content-Type': 'application/json'
        }

        # Adjust creativity based on question type
        if any(word in user_question.lower() for word in ['emergency', 'critical', 'warning', 'alert']):
            # Critical situations: Be direct and specific
            temp_setting = 0.5
            max_tokens_setting = 250
        elif any(word in user_question.lower() for word in ['recommend', 'suggest', 'optimize', 'improve', 'what could', 'creative']):
            # Creative suggestions: Be more creative
            temp_setting = 0.9
            max_tokens_setting = 400
        else:
            # Normal questions: Balanced
            temp_setting = 0.7
            max_tokens_setting = 300

        payload = {
            "model": "gpt-4o-mini",
            "messages": [
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": user_question}
            ],
            "temperature": temp_setting,
            "max_tokens": max_tokens_setting
        }

        api_response = requests.post(
            'https://api.openai.com/v1/chat/completions',
            headers=headers,
            json=payload,
            timeout=30
        )

        if api_response.status_code != 200:
            raise Exception(f"OpenAI API error: {api_response.text}")

        ai_response = api_response.json()['choices'][0]['message']['content']

        return jsonify({
            'success': True,
            'question': user_question,
            'answer': ai_response,
            'context': {
                'battery': battery_status,
                'vision': vision_status
            }
        })

    except Exception as e:
        return jsonify({'error': str(e)}), 500


# ================================
# MOBILE APP ENDPOINTS
# ================================

@app.route('/mobile/events', methods=['GET'])
def get_mobile_events():
    """Get recent critical events for mobile app - from Google Drive (last 24 hours only)"""
    try:
        events = []
        limit = int(request.args.get('limit', 20))  # Default 20 events
        hours = int(request.args.get('hours', 24))  # Default last 24 hours

        # Calculate cutoff time
        cutoff_time = datetime.datetime.now() - datetime.timedelta(hours=hours)
        print(f"📅 Fetching events from last {hours} hours (after {cutoff_time.isoformat()})")

        # Try to get from Google Drive first
        if GOOGLE_DRIVE_AVAILABLE:
            try:
                service = get_google_drive_service()
                if service:
                    folder_id = get_or_create_drive_folder(service)

                    # Search for JSON files in Drive folder (last 24 hours)
                    # Format: RFC 3339 timestamp for Google Drive query
                    cutoff_rfc3339 = cutoff_time.strftime('%Y-%m-%dT%H:%M:%S')
                    query = f"'{folder_id}' in parents and name contains 'critical_event' and trashed=false and createdTime > '{cutoff_rfc3339}'"

                    results = service.files().list(
                        q=query,
                        pageSize=100,  # Get more to filter later
                        orderBy='createdTime desc',
                        fields="files(id, name, createdTime)"
                    ).execute()

                    files = results.get('files', [])
                    print(f"📥 Found {len(files)} recent files in Google Drive")

                    # Download and parse each file, filter by timestamp
                    for file in files:
                        try:
                            file_id = file['id']
                            file_content = service.files().get_media(fileId=file_id).execute()
                            event_data = json.loads(file_content.decode('utf-8'))

                            # Double-check timestamp in event data
                            event_time = datetime.datetime.fromisoformat(event_data.get('timestamp', '').replace('Z', '+00:00'))
                            if event_time >= cutoff_time:
                                events.append(event_data)

                            if len(events) >= limit:
                                break
                        except Exception as e:
                            print(f"⚠️  Error downloading {file['name']}: {e}")
                            continue

                    print(f"✅ Loaded {len(events)} recent events from Google Drive")

                    return jsonify({
                        'success': True,
                        'events': events[:limit],
                        'count': len(events[:limit]),
                        'source': 'google_drive',
                        'hours': hours
                    })
            except Exception as e:
                print(f"⚠️  Google Drive read error: {e}")
                print("📁 Falling back to local storage...")

        # Fallback: Read from local storage (last 24 hours)
        if CRITICAL_LOGS_DIR.exists():
            all_files = sorted(CRITICAL_LOGS_DIR.glob('*.json'), key=lambda x: x.stat().st_mtime, reverse=True)

            for file in all_files:
                try:
                    # Check file modification time
                    file_time = datetime.datetime.fromtimestamp(file.stat().st_mtime)
                    if file_time < cutoff_time:
                        continue

                    with open(file, 'r', encoding='utf-8') as f:
                        event_data = json.load(f)
                        events.append(event_data)

                    if len(events) >= limit:
                        break
                except Exception as e:
                    print(f"⚠️  Error reading {file}: {e}")
                    continue

        return jsonify({
            'success': True,
            'events': events[:limit],
            'count': len(events[:limit]),
            'source': 'local_storage',
            'hours': hours
        })

    except Exception as e:
        print(f"⚠️  Mobile events error: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/mobile/latest', methods=['GET'])
def get_latest_snapshot():
    """Get latest system snapshot for mobile app"""
    try:
        # Try to get latest hourly snapshot
        if HOURLY_LOGS_DIR.exists():
            files = sorted(HOURLY_LOGS_DIR.glob('*.json'), key=lambda x: x.stat().st_mtime, reverse=True)

            if files:
                with open(files[0], 'r', encoding='utf-8') as f:
                    snapshot = json.load(f)
                    return jsonify({
                        'success': True,
                        'snapshot': snapshot
                    })

        # Fallback: Return current system state
        return jsonify({
            'success': True,
            'snapshot': {
                'battery': {
                    'cycle_count': cycle_count,
                    'capacity_fade': round(capacity_fade, 2),
                    'health_estimate': max(0, 100 - capacity_fade)
                },
                'timestamp': datetime.datetime.now().isoformat(),
                'note': 'Live data - no snapshot available'
            }
        })

    except Exception as e:
        print(f"⚠️  Mobile latest error: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/mobile/daily-summary', methods=['GET'])
def get_daily_summary():
    """Get AI-generated daily summary for mobile dashboard"""
    try:
        # Count today's events
        today = datetime.datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
        today_events = 0
        critical_count = 0

        if CRITICAL_LOGS_DIR.exists():
            for file in CRITICAL_LOGS_DIR.glob('*.json'):
                file_time = datetime.datetime.fromtimestamp(file.stat().st_mtime)
                if file_time >= today:
                    today_events += 1
                    try:
                        with open(file, 'r', encoding='utf-8') as f:
                            event_data = json.load(f)
                            if event_data.get('severity') == 'CRITICAL':
                                critical_count += 1
                    except:
                        pass

        # Generate AI summary
        if today_events == 0:
            summary = "✅ All systems running smoothly today. No critical events detected."
            icon = "check_circle"
            color = "green"
        elif critical_count == 0:
            summary = f"⚠️ {today_events} event{'s' if today_events > 1 else ''} logged today. System stable."
            icon = "info"
            color = "blue"
        else:
            summary = f"🚨 {critical_count} critical event{'s' if critical_count > 1 else ''} today! Attention needed."
            icon = "warning"
            color = "red"

        return jsonify({
            'success': True,
            'summary': {
                'text': summary,
                'icon': icon,
                'color': color,
                'total_events': today_events,
                'critical_events': critical_count,
                'date': datetime.datetime.now().isoformat()
            }
        })

    except Exception as e:
        print(f"⚠️  Daily summary error: {e}")
        return jsonify({'error': str(e)}), 500


@app.route('/mobile/stats', methods=['GET'])
def get_mobile_stats():
    """Get statistics for mobile app dashboard"""
    try:
        stats = {
            'total_critical_events': 0,
            'total_snapshots': 0,
            'last_event_time': None,
            'system_uptime_hours': 0
        }

        # Count critical events
        if CRITICAL_LOGS_DIR.exists():
            stats['total_critical_events'] = len(list(CRITICAL_LOGS_DIR.glob('*.json')))

            # Get last event time
            files = sorted(CRITICAL_LOGS_DIR.glob('*.json'), key=lambda x: x.stat().st_mtime, reverse=True)
            if files:
                with open(files[0], 'r', encoding='utf-8') as f:
                    event = json.load(f)
                    stats['last_event_time'] = event.get('timestamp')

        # Count snapshots
        if HOURLY_LOGS_DIR.exists():
            stats['total_snapshots'] = len(list(HOURLY_LOGS_DIR.glob('*.json')))

        return jsonify({
            'success': True,
            'stats': stats
        })

    except Exception as e:
        print(f"⚠️  Mobile stats error: {e}")
        return jsonify({'error': str(e)}), 500


if __name__ == '__main__':
    print("Flask sunucusu başlatılıyor...")
    print("URL: http://localhost:5000")
    app.run(host='0.0.0.0', port=5000, debug=True)
