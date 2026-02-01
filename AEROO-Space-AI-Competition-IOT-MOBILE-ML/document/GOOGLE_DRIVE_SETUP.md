# Google Drive Logging System - Setup Guide

## Overview

This guide explains how to set up Google Drive integration for automatic logging of critical events and hourly snapshots from your AEROO Solar Battery Node AI system.

## What Gets Logged

### 1. Critical Events (Immediate)
- **BATTERY_EMERGENCY**: Battery health < 20%
- **BATTERY_WARNING**: Battery health drops below 40%
- **PANEL_CRITICAL**: Panel health < 30% with 5+ anomalies
- **TEMPERATURE_ANOMALY**: Temperature < 0°C or > 45°C
- **VOLTAGE_ANOMALY**: Voltage < 3.3V or > 4.3V
- **DUST_ACCUMULATION**: Significant dust increase (> 2 areas)
- **CRACK_DETECTED**: New cracks discovered on panel
- **OBSTRUCTION_DETECTED**: New obstructions covering panel
- **ACCELERATED_DEGRADATION**: Battery health drops > 10% suddenly

### 2. Hourly Snapshots (Every Hour)
- Complete battery state (health, voltage, current, temperature, cycles)
- Complete panel state (health, dust/crack/cover counts, confidence)
- System statistics (uptime, timestamp)

## Prerequisites

- Google Account
- Python 3.7+ installed
- Flask backend running (`python_backend/app.py`)
- Internet connection for Google Drive access

## Step-by-Step Setup

### Step 1: Create Google Cloud Project

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Click **"Create Project"** or select existing project
3. Enter project name: `AEROO-Solar-Logs` (or any name)
4. Click **"Create"**

### Step 2: Enable Google Drive API

1. In Google Cloud Console, go to **"APIs & Services" > "Library"**
2. Search for **"Google Drive API"**
3. Click on it and press **"Enable"**
4. Wait for API to be enabled (takes a few seconds)

### Step 3: Create OAuth 2.0 Credentials

1. Go to **"APIs & Services" > "Credentials"**
2. Click **"+ CREATE CREDENTIALS"** at the top
3. Select **"OAuth client ID"**

4. **If prompted to configure OAuth consent screen:**
   - Click **"CONFIGURE CONSENT SCREEN"**
   - Select **"External"** (unless you have Google Workspace)
   - Click **"CREATE"**
   - Fill in required fields:
     - App name: `AEROO Solar Logger`
     - User support email: Your email
     - Developer contact: Your email
   - Click **"SAVE AND CONTINUE"**
   - Skip "Scopes" section (click **"SAVE AND CONTINUE"**)
   - Add test users (your email address)
   - Click **"SAVE AND CONTINUE"**
   - Click **"BACK TO DASHBOARD"**

5. **Create OAuth Client ID:**
   - Go back to **"Credentials"** tab
   - Click **"+ CREATE CREDENTIALS" > "OAuth client ID"**
   - Application type: **"Desktop app"**
   - Name: `AEROO Solar Logger Desktop`
   - Click **"CREATE"**

6. **Download Credentials:**
   - A popup will show your Client ID and Secret
   - Click **"DOWNLOAD JSON"**
   - Save the file

### Step 4: Install Credentials File

1. Rename the downloaded file to `credentials.json`
2. Move it to your project's `python_backend/` folder:
   ```
   C:\ST\workspace\WiFi_HTTP_Server\python_backend\credentials.json
   ```

**IMPORTANT:** This file contains sensitive information. It's already protected by `.gitignore` and won't be uploaded to GitHub.

### Step 5: Install Required Python Packages

Open terminal in `python_backend/` folder and run:

```bash
cd python_backend
pip install -r requirements.txt
```

This installs:
- `google-api-python-client` - Google Drive API client
- `google-auth-httplib2` - HTTP library for Google Auth
- `google-auth-oauthlib` - OAuth 2.0 authentication flow

### Step 6: Run First Authentication

1. Start your Flask backend:
   ```bash
   python app.py
   ```

2. **First time only:** A browser window will open automatically
3. **Sign in** to your Google Account
4. **Grant permissions** when prompted:
   - "AEROO Solar Logger wants to access your Google Account"
   - Click **"Continue"**
   - Review permissions (upload files to Drive)
   - Click **"Continue"** or **"Allow"**

5. You'll see: **"The authentication flow has completed. You may close this window."**

6. A `token.pickle` file is now created in `python_backend/`
   - This stores your authentication token
   - You won't need to authenticate again unless you delete it
   - It's protected by `.gitignore`

### Step 7: Verify Setup

1. Check backend console for:
   ```
   ✅ Google Drive API initialized successfully
   📁 Drive folder ready: AEROO_Solar_Logs
   ```

2. Open your dashboard: `index.html`

3. Trigger a test event (or wait for real data):
   - Battery health drops below 40%
   - Temperature anomaly detected
   - Panel anomaly detected

4. Check backend console for:
   ```
   📝 Critical event logged: critical_event_2026-01-06_12-30-45.json
   ✅ Uploaded: critical_event_2026-01-06_12-30-45.json
   ```

5. **Verify in Google Drive:**
   - Go to [Google Drive](https://drive.google.com/)
   - Look for folder: **"AEROO_Solar_Logs"**
   - You should see log files inside

## Folder Structure in Google Drive

```
AEROO_Solar_Logs/
├── critical_event_2026-01-06_12-30-45.json
├── critical_event_2026-01-06_14-15-20.json
├── hourly_snapshot_2026-01-06_13-00-00.json
├── hourly_snapshot_2026-01-06_14-00-00.json
└── ...
```

## Log File Format

### Critical Event Example
```json
{
  "event_type": "BATTERY_EMERGENCY",
  "severity": "CRITICAL",
  "timestamp": "2026-01-06T12:30:45.123456",
  "battery": {
    "health": 18.5,
    "voltage": 3.45,
    "current": 1250,
    "temperature": 42,
    "cycle_count": 850,
    "capacity_fade": 81.5
  },
  "panel": {
    "health": 75,
    "dust_count": 3,
    "crack_count": 1,
    "cover_count": 0
  },
  "ai_alert": "🚨 EMERGENCY: Battery at 18.5%! Immediate replacement needed...",
  "logged_at": "2026-01-06T12:30:45.234567"
}
```

### Hourly Snapshot Example
```json
{
  "snapshot_type": "HOURLY_SNAPSHOT",
  "timestamp": "2026-01-06T13:00:00.123456",
  "battery": {
    "health": 45.2,
    "voltage": 3.78,
    "current": 1100,
    "temperature": 35,
    "cycle_count": 845,
    "capacity_fade": 54.8,
    "rul_cycles": 155
  },
  "panel": {
    "health": 85,
    "dust_count": 2,
    "crack_count": 0,
    "cover_count": 0,
    "confidence_avg": 0.92
  },
  "statistics": {
    "snapshot_timestamp": "2026-01-06T13:00:00.000Z",
    "uptime_hours": 5
  },
  "logged_at": "2026-01-06T13:00:00.234567"
}
```

## Local Backup System

Even if Google Drive is unavailable, logs are **always saved locally**:

```
python_backend/
├── logs/
│   ├── critical_events/
│   │   ├── critical_event_2026-01-06_12-30-45.json
│   │   └── ...
│   └── hourly_snapshots/
│       ├── hourly_snapshot_2026-01-06_13-00-00.json
│       └── ...
```

Local logs are protected by `.gitignore` and won't be uploaded to GitHub.

## Troubleshooting

### Issue 1: "credentials.json not found"

**Problem:** Backend shows `⚠️ credentials.json not found. Google Drive disabled.`

**Solution:**
1. Download credentials from Google Cloud Console (Step 3)
2. Rename to `credentials.json`
3. Place in `python_backend/` folder
4. Restart backend: `python app.py`

### Issue 2: "Invalid credentials" or "token expired"

**Problem:** Authentication fails or shows expired token error.

**Solution:**
1. Delete `token.pickle` file in `python_backend/`
2. Restart backend: `python app.py`
3. Browser will open - sign in again
4. Grant permissions

### Issue 3: Browser doesn't open for authentication

**Problem:** First run doesn't open browser window.

**Solution:**
1. Check console for URL like: `http://localhost:xxxxx/?code=...`
2. Copy and paste URL into browser manually
3. Complete authentication flow
4. Return to terminal - should show success

### Issue 4: "Access blocked: This app isn't verified"

**Problem:** Google shows warning during authentication.

**Solution:**
1. This is normal for development apps
2. Click **"Advanced"** (bottom left)
3. Click **"Go to AEROO Solar Logger (unsafe)"**
4. This is YOUR app, it's safe
5. Continue with authentication

### Issue 5: Files not appearing in Google Drive

**Problem:** Backend says "uploaded" but files not in Drive.

**Solution:**
1. Check backend console for actual upload confirmation:
   ```
   ✅ Uploaded: filename.json
   ```
2. Refresh Google Drive page (F5)
3. Check "AEROO_Solar_Logs" folder specifically
4. Try searching Drive for "critical_event" or "hourly_snapshot"
5. Verify internet connection

### Issue 6: "Rate limit exceeded"

**Problem:** Too many API calls, uploads failing.

**Solution:**
1. Google Drive API has quotas (10,000 requests/day for free)
2. Our system is well within limits (24 hourly + ~10 critical = ~34/day)
3. If you hit limit, files still save locally in `logs/` folder
4. Wait 24 hours for quota reset
5. Consider upgrading Google Cloud project if needed

### Issue 7: Local logs folder missing

**Problem:** `logs/` folder doesn't exist.

**Solution:**
1. Folders are auto-created on first run
2. If missing, backend will create them automatically
3. Check backend console for: `📁 Logs directory created`
4. Restart backend if needed

## Security Best Practices

### Files Protected by .gitignore
- `credentials.json` - OAuth client credentials
- `token.pickle` - Authentication token
- `logs/` folder - All local log files
- `*.log` - Debug log files

### Never Commit These Files
These files contain sensitive information and should NEVER be uploaded to GitHub:
1. `credentials.json` - Can access your Google account
2. `token.pickle` - Active authentication session
3. Log files - May contain system data

### Sharing Credentials
If working with a team:
1. Each developer should create their own `credentials.json`
2. Or share via secure channel (not Git/email)
3. Each person authenticates individually

## API Endpoints

### POST /log-critical-event
Logs critical system events immediately.

**Request:**
```json
{
  "event_type": "BATTERY_EMERGENCY",
  "severity": "CRITICAL",
  "battery": { ... },
  "panel": { ... },
  "ai_alert": "..."
}
```

**Response:**
```json
{
  "success": true,
  "message": "Critical event logged successfully",
  "file_path": "logs/critical_events/critical_event_2026-01-06_12-30-45.json",
  "uploaded_to_drive": true,
  "timestamp": "2026-01-06T12:30:45.123456"
}
```

### POST /log-hourly-snapshot
Logs periodic system snapshots (called every hour by frontend timer).

**Request:**
```json
{
  "battery": { ... },
  "panel": { ... },
  "statistics": { ... }
}
```

**Response:**
```json
{
  "success": true,
  "message": "Hourly snapshot logged successfully",
  "file_path": "logs/hourly_snapshots/hourly_snapshot_2026-01-06_13-00-00.json",
  "uploaded_to_drive": true,
  "timestamp": "2026-01-06T13:00:00.123456"
}
```

## Monitoring and Maintenance

### Check Logged Events
1. **Google Drive:** Browse `AEROO_Solar_Logs` folder
2. **Local:** Check `python_backend/logs/` folders
3. **Backend Console:** Look for `✅ Uploaded:` messages
4. **Frontend Console:** Look for `📊 Hourly snapshot logged:` messages

### Disk Space Management
- Each log file is ~1-2 KB
- 24 hourly snapshots/day = ~24-48 KB/day
- Critical events vary (estimated 5-10/day = ~10-20 KB/day)
- Total: ~40-70 KB/day
- **1 GB will last ~40 years** at this rate

Local logs can be archived or deleted manually if needed. Google Drive files can be deleted anytime.

### Google Drive Storage Quota
- Free Google account: 15 GB
- At 70 KB/day, you can log for **214,000+ days** (586+ years)
- No storage concerns for this application

## Competition Notes

For the AEROO Space AI Competition jury:

### Why Google Drive?
1. **Persistent Storage:** Data survives system restarts/crashes
2. **Remote Access:** Review logs from any device
3. **Backup Safety:** Hardware failure won't lose data
4. **Professional:** Industry-standard cloud logging
5. **Competition Ready:** Demonstrable data collection

### Data Analysis Benefits
- Download all logs as JSON for analysis
- Import into pandas/Excel for visualization
- Track battery degradation over time
- Correlate temperature with battery health
- Identify panel maintenance patterns

### Demonstration During Jury Review
1. Show Google Drive folder with logs
2. Open a critical event JSON file
3. Explain timestamp correlation with AI alerts
4. Show hourly snapshot for system health trends
5. Demonstrate local backup system

## Additional Resources

- [Google Drive API Documentation](https://developers.google.com/drive/api/v3/about-sdk)
- [OAuth 2.0 Guide](https://developers.google.com/identity/protocols/oauth2)
- [Google Cloud Console](https://console.cloud.google.com/)
- [Flask Documentation](https://flask.palletsprojects.com/)

## Support

If you encounter issues not covered in this guide:

1. Check backend console for error messages
2. Check frontend console (F12) for JavaScript errors
3. Verify `credentials.json` and `token.pickle` exist
4. Try deleting `token.pickle` and re-authenticating
5. Ensure internet connection is stable
6. Check Google Cloud Console for API quota status

## Summary

Your AEROO Solar Battery Node AI system now has:
- ✅ Automatic critical event logging
- ✅ Hourly system snapshots
- ✅ Google Drive cloud backup
- ✅ Local backup system
- ✅ 9 types of critical event detection
- ✅ Complete battery and panel state tracking
- ✅ Professional data logging for competition

All logs are timestamped, structured (JSON), and ready for analysis. The system operates autonomously once configured.
