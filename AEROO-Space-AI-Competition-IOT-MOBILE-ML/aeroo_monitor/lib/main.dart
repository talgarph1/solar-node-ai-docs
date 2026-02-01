import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'dart:convert';
import 'package:intl/intl.dart';

// Simple translation system
class AppTranslations {
  static final Map<String, Map<String, String>> _translations = {
    'en': {
      'app_title': 'AEROO Solar Monitor',
      'solar_battery_monitor': 'Solar Battery Monitor',
      'critical_events': 'Critical Events',
      'snapshots': 'Snapshots',
      'recent_critical_events': 'Recent Critical Events',
      'no_events': 'No events yet',
      'all_systems_smooth': 'All systems are running smoothly',
      'connection_error': 'Connection Error',
      'try_again': 'Try Again',
      'battery': 'Battery',
      'panel': 'Panel',
      'temp': 'Temp',
      'dust': 'Dust',
      'cracks': 'Cracks',
      'coverage': 'Coverage',
      'event_details': 'Event Details',
      'battery_status': '🔋 Battery Status',
      'panel_status': '☀️ Panel Status',
      'ai_alert': '💬 AI Alert',
      'health': 'Health',
      'voltage': 'Voltage',
      'current': 'Current',
      'temperature': 'Temperature',
      'cycle_count': 'Cycle Count',
      'capacity_fade': 'Capacity Fade',
      'dust_areas': 'Dust Areas',
      'just_now': 'Just now',
      'ago': 'ago',
    },
    'ru': {
      'app_title': 'AEROO Монитор',
      'solar_battery_monitor': 'Монитор солнечной батареи',
      'critical_events': 'Критические события',
      'snapshots': 'Снимки',
      'recent_critical_events': 'Недавние критические события',
      'no_events': 'Событий пока нет',
      'all_systems_smooth': 'Все системы работают нормально',
      'connection_error': 'Ошибка подключения',
      'try_again': 'Попробовать снова',
      'battery': 'Батарея',
      'panel': 'Панель',
      'temp': 'Темп',
      'dust': 'Пыль',
      'cracks': 'Трещины',
      'coverage': 'Покрытие',
      'event_details': 'Детали события',
      'battery_status': '🔋 Состояние батареи',
      'panel_status': '☀️ Состояние панели',
      'ai_alert': '💬 Предупреждение ИИ',
      'health': 'Здоровье',
      'voltage': 'Напряжение',
      'current': 'Ток',
      'temperature': 'Температура',
      'cycle_count': 'Циклы',
      'capacity_fade': 'Износ',
      'dust_areas': 'Пыльные зоны',
      'just_now': 'Только что',
      'ago': 'назад',
    },
  };

  static String locale = 'en';

  static String get(String key) {
    return _translations[locale]?[key] ?? _translations['en']?[key] ?? key;
  }

  static void setLocale(String newLocale) {
    if (_translations.containsKey(newLocale)) {
      locale = newLocale;
    }
  }
}

void main() {
  runApp(const AerooMonitorApp());
}

class AerooMonitorApp extends StatefulWidget {
  const AerooMonitorApp({super.key});

  static _AerooMonitorAppState? of(BuildContext context) {
    return context.findAncestorStateOfType<_AerooMonitorAppState>();
  }

  @override
  State<AerooMonitorApp> createState() => _AerooMonitorAppState();
}

class _AerooMonitorAppState extends State<AerooMonitorApp> {
  void setLocale(String locale) {
    setState(() {
      AppTranslations.setLocale(locale);
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: AppTranslations.get('app_title'),
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xFF1E88E5)),
        useMaterial3: true,
        scaffoldBackgroundColor: const Color(0xFFF5F7FA),
      ),
      home: const EventListScreen(),
    );
  }
}

class EventListScreen extends StatefulWidget {
  const EventListScreen({super.key});

  @override
  State<EventListScreen> createState() => _EventListScreenState();
}

class _EventListScreenState extends State<EventListScreen> {
  List<dynamic> events = [];
  Map<String, dynamic>? stats;
  Map<String, dynamic>? dailySummary;
  bool isLoading = true;
  String? error;

  // Backend URL - For Real Device (APK)
  // Use your PC's network IP address
  // Options: http://10.215.117.73:5000 or http://192.168.10.10:5000
  // For Emulator: Use http://localhost:5000 with adb reverse
  static const String backendUrl = 'http://10.215.117.73:5000';

  @override
  void initState() {
    super.initState();
    loadData();
  }

  Future<void> loadData() async {
    setState(() {
      isLoading = true;
      error = null;
    });

    try {
      print('🔗 Attempting to connect to: $backendUrl');

      final eventsResponse = await http.get(
        Uri.parse('$backendUrl/mobile/events?limit=20'),
      ).timeout(const Duration(seconds: 30));

      print('✅ Events response: ${eventsResponse.statusCode}');

      final statsResponse = await http.get(
        Uri.parse('$backendUrl/mobile/stats'),
      ).timeout(const Duration(seconds: 10));

      print('✅ Stats response: ${statsResponse.statusCode}');

      final summaryResponse = await http.get(
        Uri.parse('$backendUrl/mobile/daily-summary'),
      ).timeout(const Duration(seconds: 10));

      print('✅ Summary response: ${summaryResponse.statusCode}');

      if (eventsResponse.statusCode == 200 && statsResponse.statusCode == 200) {
        final eventsData = json.decode(eventsResponse.body);
        final statsData = json.decode(statsResponse.body);
        final summaryData = summaryResponse.statusCode == 200
            ? json.decode(summaryResponse.body)
            : null;

        setState(() {
          events = eventsData['events'] ?? [];
          stats = statsData['stats'];
          dailySummary = summaryData?['summary'];
          isLoading = false;
        });
        print('✅ Data loaded successfully: ${events.length} events');
      } else {
        throw Exception('Server returned error: Events=${eventsResponse.statusCode}, Stats=${statsResponse.statusCode}');
      }
    } catch (e) {
      print('❌ Connection error: $e');
      String detailedError = '';
      if (e.toString().contains('SocketException')) {
        detailedError = '⚠️ Cannot reach server\n\n'
            'Current URL: $backendUrl\n\n'
            'Tips:\n'
            '• Emulator: Use http://10.0.2.2:5000\n'
            '• Real device: Use your PC IP\n'
            '• Check if backend is running\n'
            '• Check Settings (⚙️) to update URL';
      } else if (e.toString().contains('TimeoutException')) {
        detailedError = '⏱️ Connection timeout\n\n'
            'Server is not responding at:\n$backendUrl\n\n'
            'Please check Settings (⚙️) and ensure\nbackend is running on port 5000';
      } else {
        detailedError = 'Connection failed:\n${e.toString()}';
      }

      setState(() {
        error = detailedError;
        isLoading = false;
      });
    }
  }

  Color _getSeverityColor(String severity) {
    switch (severity.toUpperCase()) {
      case 'CRITICAL':
        return const Color(0xFFFF5252);
      case 'WARNING':
        return const Color(0xFFFF9800);
      default:
        return const Color(0xFF2196F3);
    }
  }

  IconData _getEventIcon(String eventType) {
    if (eventType.contains('BATTERY')) {
      return Icons.battery_alert_rounded;
    } else if (eventType.contains('PANEL')) {
      return Icons.solar_power_rounded;
    } else if (eventType.contains('TEMPERATURE')) {
      return Icons.thermostat_rounded;
    } else if (eventType.contains('VOLTAGE')) {
      return Icons.electric_bolt_rounded;
    } else {
      return Icons.warning_rounded;
    }
  }

  String _formatTimestamp(String timestamp) {
    try {
      // Parse UTC timestamp and convert to local time
      final dt = DateTime.parse(timestamp).toLocal();
      final now = DateTime.now();
      final difference = now.difference(dt);

      if (difference.inMinutes < 1) {
        return 'Just now';
      } else if (difference.inMinutes < 60) {
        return '${difference.inMinutes}m ago';
      } else if (difference.inHours < 24) {
        return '${difference.inHours}h ago';
      } else {
        return DateFormat('MMM d, HH:mm').format(dt);
      }
    } catch (e) {
      return timestamp;
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [Color(0xFF1E88E5), Color(0xFF1565C0), Color(0xFFF5F7FA)],
            stops: [0.0, 0.25, 0.25],
          ),
        ),
        child: SafeArea(
          child: Column(
            children: [
              _buildHeader(context),
              Expanded(child: _buildBody(context)),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildHeader(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(20),
      child: Row(
        children: [
          Container(
            padding: const EdgeInsets.all(12),
            decoration: BoxDecoration(
              color: Colors.white.withOpacity(0.2),
              borderRadius: BorderRadius.circular(16),
              border: Border.all(color: Colors.white.withOpacity(0.3), width: 1.5),
            ),
            child: const Icon(Icons.solar_power_rounded, color: Colors.white, size: 32),
          ),
          const SizedBox(width: 16),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text(
                  'AEROO',
                  style: TextStyle(
                    color: Colors.white,
                    fontSize: 26,
                    fontWeight: FontWeight.bold,
                    letterSpacing: 1.2,
                  ),
                ),
                Text(
                  AppTranslations.get('solar_battery_monitor'),
                  style: const TextStyle(
                    color: Colors.white70,
                    fontSize: 13,
                    fontWeight: FontWeight.w500,
                  ),
                ),
              ],
            ),
          ),
          // Language switcher
          Container(
            decoration: BoxDecoration(
              color: Colors.white.withOpacity(0.2),
              borderRadius: BorderRadius.circular(12),
            ),
            child: PopupMenuButton<String>(
              icon: const Icon(Icons.language, color: Colors.white, size: 24),
              onSelected: (String locale) {
                AerooMonitorApp.of(context)?.setLocale(locale);
                setState(() {}); // Refresh current screen
              },
              itemBuilder: (BuildContext context) => [
                const PopupMenuItem(
                  value: 'en',
                  child: Text('🇬🇧 English'),
                ),
                const PopupMenuItem(
                  value: 'ru',
                  child: Text('🇷🇺 Русский'),
                ),
              ],
            ),
          ),
          const SizedBox(width: 8),
          Container(
            decoration: BoxDecoration(
              color: Colors.white.withOpacity(0.2),
              borderRadius: BorderRadius.circular(12),
            ),
            child: IconButton(
              icon: const Icon(Icons.refresh_rounded, color: Colors.white, size: 24),
              onPressed: loadData,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildBody(BuildContext context) {
    return Container(
      decoration: const BoxDecoration(
        color: Color(0xFFF5F7FA),
        borderRadius: BorderRadius.only(
          topLeft: Radius.circular(32),
          topRight: Radius.circular(32),
        ),
      ),
      child: isLoading
          ? const Center(
              child: CircularProgressIndicator(
                valueColor: AlwaysStoppedAnimation<Color>(Color(0xFF1E88E5)),
                strokeWidth: 3,
              ),
            )
          : error != null
              ? Center(
                  child: Padding(
                    padding: const EdgeInsets.all(32),
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Container(
                          padding: const EdgeInsets.all(24),
                          decoration: BoxDecoration(
                            color: Colors.red.shade50,
                            shape: BoxShape.circle,
                          ),
                          child: Icon(
                            Icons.cloud_off_rounded,
                            size: 64,
                            color: Colors.red.shade400,
                          ),
                        ),
                        const SizedBox(height: 24),
                        Text(
                          AppTranslations.get('connection_error'),
                          style: const TextStyle(
                            fontSize: 22,
                            fontWeight: FontWeight.bold,
                            color: Color(0xFF2C3E50),
                          ),
                        ),
                        const SizedBox(height: 12),
                        Text(
                          error!,
                          textAlign: TextAlign.center,
                          style: TextStyle(
                            fontSize: 14,
                            color: Colors.grey.shade600,
                          ),
                        ),
                        const SizedBox(height: 32),
                        ElevatedButton.icon(
                          onPressed: loadData,
                          icon: const Icon(Icons.refresh_rounded),
                          label: Text(AppTranslations.get('try_again')),
                          style: ElevatedButton.styleFrom(
                            backgroundColor: const Color(0xFF1E88E5),
                            foregroundColor: Colors.white,
                            padding: const EdgeInsets.symmetric(
                              horizontal: 32,
                              vertical: 16,
                            ),
                            shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(16),
                            ),
                            elevation: 0,
                          ),
                        ),
                      ],
                    ),
                  ),
                )
              : RefreshIndicator(
                  onRefresh: loadData,
                  color: const Color(0xFF1E88E5),
                  child: ListView(
                    padding: const EdgeInsets.all(24),
                    children: [
                      // Daily AI Summary
                      if (dailySummary != null) ...[
                        Container(
                          padding: const EdgeInsets.all(20),
                          decoration: BoxDecoration(
                            gradient: LinearGradient(
                              colors: dailySummary!['color'] == 'green'
                                  ? [const Color(0xFF4CAF50), const Color(0xFF45A049)]
                                  : dailySummary!['color'] == 'red'
                                      ? [const Color(0xFFFF6B6B), const Color(0xFFEE5A6F)]
                                      : [const Color(0xFF2196F3), const Color(0xFF1976D2)],
                              begin: Alignment.topLeft,
                              end: Alignment.bottomRight,
                            ),
                            borderRadius: BorderRadius.circular(20),
                            boxShadow: [
                              BoxShadow(
                                color: (dailySummary!['color'] == 'green'
                                        ? const Color(0xFF4CAF50)
                                        : dailySummary!['color'] == 'red'
                                            ? const Color(0xFFFF6B6B)
                                            : const Color(0xFF2196F3))
                                    .withOpacity(0.3),
                                blurRadius: 12,
                                offset: const Offset(0, 6),
                              ),
                            ],
                          ),
                          child: Row(
                            children: [
                              Container(
                                padding: const EdgeInsets.all(12),
                                decoration: BoxDecoration(
                                  color: Colors.white.withOpacity(0.25),
                                  borderRadius: BorderRadius.circular(16),
                                ),
                                child: Icon(
                                  dailySummary!['icon'] == 'check_circle'
                                      ? Icons.check_circle_rounded
                                      : dailySummary!['icon'] == 'warning'
                                          ? Icons.warning_rounded
                                          : Icons.info_rounded,
                                  color: Colors.white,
                                  size: 32,
                                ),
                              ),
                              const SizedBox(width: 16),
                              Expanded(
                                child: Text(
                                  dailySummary!['text'] ?? '',
                                  style: const TextStyle(
                                    color: Colors.white,
                                    fontSize: 15,
                                    fontWeight: FontWeight.w600,
                                    height: 1.4,
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ),
                        const SizedBox(height: 20),
                      ],

                      // Stats Cards
                      if (stats != null) ...[
                        Row(
                          children: [
                            Expanded(
                              child: _buildModernStatCard(
                                AppTranslations.get('critical_events'),
                                stats!['total_critical_events'].toString(),
                                Icons.warning_rounded,
                                const LinearGradient(
                                  colors: [Color(0xFFFF6B6B), Color(0xFFEE5A6F)],
                                  begin: Alignment.topLeft,
                                  end: Alignment.bottomRight,
                                ),
                              ),
                            ),
                            const SizedBox(width: 16),
                            Expanded(
                              child: _buildModernStatCard(
                                AppTranslations.get('snapshots'),
                                stats!['total_snapshots'].toString(),
                                Icons.camera_alt_rounded,
                                const LinearGradient(
                                  colors: [Color(0xFF4CAF50), Color(0xFF45A049)],
                                  begin: Alignment.topLeft,
                                  end: Alignment.bottomRight,
                                ),
                              ),
                            ),
                          ],
                        ),
                        const SizedBox(height: 28),
                      ],

                      // Events Header
                      Row(
                        children: [
                          Container(
                            padding: const EdgeInsets.all(10),
                            decoration: BoxDecoration(
                              color: const Color(0xFF1E88E5).withOpacity(0.12),
                              borderRadius: BorderRadius.circular(12),
                            ),
                            child: const Icon(
                              Icons.history_rounded,
                              color: Color(0xFF1E88E5),
                              size: 22,
                            ),
                          ),
                          const SizedBox(width: 14),
                          Text(
                            AppTranslations.get('recent_critical_events'),
                            style: const TextStyle(
                              fontSize: 19,
                              fontWeight: FontWeight.bold,
                              color: Color(0xFF2C3E50),
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 18),

                      // Events List
                      if (events.isEmpty)
                        Center(
                          child: Padding(
                            padding: const EdgeInsets.symmetric(vertical: 64),
                            child: Column(
                              children: [
                                Container(
                                  padding: const EdgeInsets.all(28),
                                  decoration: BoxDecoration(
                                    color: Colors.grey.shade100,
                                    shape: BoxShape.circle,
                                  ),
                                  child: Icon(
                                    Icons.inbox_outlined,
                                    size: 64,
                                    color: Colors.grey.shade400,
                                  ),
                                ),
                                const SizedBox(height: 20),
                                Text(
                                  'No events yet',
                                  style: TextStyle(
                                    fontSize: 17,
                                    fontWeight: FontWeight.w500,
                                    color: Colors.grey.shade600,
                                  ),
                                ),
                                const SizedBox(height: 8),
                                Text(
                                  'All systems are running smoothly',
                                  style: TextStyle(
                                    fontSize: 14,
                                    color: Colors.grey.shade500,
                                  ),
                                ),
                              ],
                            ),
                          ),
                        )
                      else
                        ...events.map((event) => _buildModernEventCard(event)),
                    ],
                  ),
                ),
    );
  }

  Widget _buildModernStatCard(
    String title,
    String value,
    IconData icon,
    Gradient gradient,
  ) {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        gradient: gradient,
        borderRadius: BorderRadius.circular(20),
        boxShadow: [
          BoxShadow(
            color: gradient.colors.first.withOpacity(0.3),
            blurRadius: 12,
            offset: const Offset(0, 6),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Container(
            padding: const EdgeInsets.all(10),
            decoration: BoxDecoration(
              color: Colors.white.withOpacity(0.25),
              borderRadius: BorderRadius.circular(12),
            ),
            child: Icon(icon, color: Colors.white, size: 24),
          ),
          const SizedBox(height: 16),
          Text(
            value,
            style: const TextStyle(
              fontSize: 28,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 4),
          Text(
            title,
            style: TextStyle(
              fontSize: 13,
              color: Colors.white.withOpacity(0.9),
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildModernEventCard(dynamic event) {
    final eventType = event['event_type'] ?? 'UNKNOWN';
    final severity = event['severity'] ?? 'INFO';
    final timestamp = event['timestamp'] ?? '';
    final battery = event['battery'] ?? {};
    final panel = event['panel'] ?? {};

    return Container(
      margin: const EdgeInsets.only(bottom: 16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(20),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.04),
            blurRadius: 10,
            offset: const Offset(0, 4),
          ),
        ],
      ),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          borderRadius: BorderRadius.circular(20),
          onTap: () {
            Navigator.push(
              context,
              MaterialPageRoute(
                builder: (context) => EventDetailScreen(event: event),
              ),
            );
          },
          child: Padding(
            padding: const EdgeInsets.all(20),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Container(
                      padding: const EdgeInsets.all(12),
                      decoration: BoxDecoration(
                        color: _getSeverityColor(severity).withOpacity(0.15),
                        borderRadius: BorderRadius.circular(14),
                      ),
                      child: Icon(
                        _getEventIcon(eventType),
                        color: _getSeverityColor(severity),
                        size: 24,
                      ),
                    ),
                    const SizedBox(width: 14),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            eventType.replaceAll('_', ' '),
                            style: const TextStyle(
                              fontSize: 16,
                              fontWeight: FontWeight.bold,
                              color: Color(0xFF2C3E50),
                            ),
                          ),
                          const SizedBox(height: 4),
                          Row(
                            children: [
                              Icon(
                                Icons.access_time_rounded,
                                size: 14,
                                color: Colors.grey.shade500,
                              ),
                              const SizedBox(width: 4),
                              Text(
                                _formatTimestamp(timestamp),
                                style: TextStyle(
                                  fontSize: 13,
                                  color: Colors.grey.shade600,
                                ),
                              ),
                            ],
                          ),
                        ],
                      ),
                    ),
                    Container(
                      padding: const EdgeInsets.symmetric(
                        horizontal: 12,
                        vertical: 6,
                      ),
                      decoration: BoxDecoration(
                        color: _getSeverityColor(severity).withOpacity(0.15),
                        borderRadius: BorderRadius.circular(8),
                      ),
                      child: Text(
                        severity,
                        style: TextStyle(
                          fontSize: 11,
                          color: _getSeverityColor(severity),
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 18),
                // Main metrics
                Container(
                  padding: const EdgeInsets.all(16),
                  decoration: BoxDecoration(
                    color: Colors.grey.shade50,
                    borderRadius: BorderRadius.circular(14),
                  ),
                  child: Row(
                    children: [
                      Expanded(
                        child: _buildMetricItem(
                          Icons.battery_charging_full_rounded,
                          'Battery',
                          '${battery['health']?.toStringAsFixed(1) ?? '?'}%',
                          const Color(0xFF2196F3),
                        ),
                      ),
                      Container(
                        width: 1,
                        height: 40,
                        color: Colors.grey.shade300,
                      ),
                      Expanded(
                        child: _buildMetricItem(
                          Icons.solar_power_rounded,
                          'Panel',
                          '${panel['health']?.toString() ?? '?'}%',
                          const Color(0xFFFF9800),
                        ),
                      ),
                      Container(
                        width: 1,
                        height: 40,
                        color: Colors.grey.shade300,
                      ),
                      Expanded(
                        child: _buildMetricItem(
                          Icons.thermostat_rounded,
                          'Temp',
                          '${battery['temperature']?.toString() ?? '?'}°C',
                          const Color(0xFFFF5252),
                        ),
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 12),
                // Visual Analysis Details
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                  decoration: BoxDecoration(
                    gradient: LinearGradient(
                      colors: [Colors.orange.shade50, Colors.amber.shade50],
                    ),
                    borderRadius: BorderRadius.circular(14),
                    border: Border.all(
                      color: Colors.orange.shade200,
                      width: 1,
                    ),
                  ),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.spaceAround,
                    children: [
                      _buildPanelIssue(
                        Icons.blur_on,
                        'Dust',
                        panel['dust_count']?.toString() ?? '0',
                        Colors.brown.shade600,
                      ),
                      Container(
                        width: 1,
                        height: 30,
                        color: Colors.orange.shade200,
                      ),
                      _buildPanelIssue(
                        Icons.broken_image_outlined,
                        'Cracks',
                        panel['crack_count']?.toString() ?? '0',
                        Colors.red.shade600,
                      ),
                      Container(
                        width: 1,
                        height: 30,
                        color: Colors.orange.shade200,
                      ),
                      _buildPanelIssue(
                        Icons.layers_outlined,
                        'Coverage',
                        panel['cover_count']?.toString() ?? '0',
                        Colors.grey.shade700,
                      ),
                    ],
                  ),
                ),
                // AI Short Summary
                if ((event['ai_summary'] != null && event['ai_summary'].toString().isNotEmpty) ||
                    (event['ai_alert'] != null && event['ai_alert'].toString().isNotEmpty))
                  Padding(
                    padding: const EdgeInsets.only(top: 12),
                    child: Container(
                      padding: const EdgeInsets.all(12),
                      decoration: BoxDecoration(
                        gradient: LinearGradient(
                          colors: [Colors.blue.shade50, Colors.lightBlue.shade50],
                        ),
                        borderRadius: BorderRadius.circular(12),
                        border: Border.all(
                          color: Colors.blue.shade200,
                          width: 1,
                        ),
                      ),
                      child: Row(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Icon(
                            Icons.psychology_rounded,
                            size: 18,
                            color: Colors.blue.shade700,
                          ),
                          const SizedBox(width: 8),
                          Expanded(
                            child: Text(
                              _getShortAISummary(event),
                              style: TextStyle(
                                fontSize: 12,
                                color: Colors.blue.shade900,
                                height: 1.3,
                              ),
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildMetricItem(IconData icon, String label, String value, Color color) {
    return Column(
      children: [
        Icon(icon, size: 20, color: color),
        const SizedBox(height: 6),
        Text(
          value,
          style: const TextStyle(
            fontSize: 15,
            fontWeight: FontWeight.bold,
            color: Color(0xFF2C3E50),
          ),
        ),
        const SizedBox(height: 2),
        Text(
          label,
          style: TextStyle(
            fontSize: 11,
            color: Colors.grey.shade600,
          ),
        ),
      ],
    );
  }

  Widget _buildPanelIssue(IconData icon, String label, String count, Color color) {
    return Column(
      children: [
        Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(icon, size: 16, color: color),
            const SizedBox(width: 4),
            Text(
              count,
              style: TextStyle(
                fontSize: 16,
                fontWeight: FontWeight.bold,
                color: color,
              ),
            ),
          ],
        ),
        const SizedBox(height: 4),
        Text(
          label,
          style: TextStyle(
            fontSize: 11,
            color: Colors.grey.shade700,
          ),
        ),
      ],
    );
  }

  String _getShortAISummary(dynamic event) {
    // Prefer ai_summary from backend
    if (event is Map && event['ai_summary'] != null) {
      return event['ai_summary'].toString();
    }

    // Fallback: extract first 2 sentences from ai_alert
    String fullAlert = '';
    if (event is Map && event['ai_alert'] != null) {
      fullAlert = event['ai_alert'].toString();
    } else if (event is String) {
      fullAlert = event;
    }

    if (fullAlert.isEmpty) return 'No AI analysis available';

    // Extract first 2 sentences
    final sentences = fullAlert.split(RegExp(r'[.!?]'));
    final firstTwo = sentences.take(2).where((s) => s.trim().isNotEmpty).join('. ');
    return firstTwo.isNotEmpty ? '$firstTwo.' : fullAlert;
  }
}

// Event Detail Screen with modern design
class EventDetailScreen extends StatelessWidget {
  final dynamic event;

  const EventDetailScreen({super.key, required this.event});

  Color _getSeverityColor(String severity) {
    switch (severity.toUpperCase()) {
      case 'CRITICAL':
        return const Color(0xFFFF5252);
      case 'WARNING':
        return const Color(0xFFFF9800);
      default:
        return const Color(0xFF2196F3);
    }
  }

  String _formatFullTimestamp(String timestamp) {
    try {
      final dt = DateTime.parse(timestamp).toLocal();
      return DateFormat('MMM d, yyyy - HH:mm:ss').format(dt);
    } catch (e) {
      return timestamp;
    }
  }

  @override
  Widget build(BuildContext context) {
    final eventType = event['event_type'] ?? 'UNKNOWN';
    final severity = event['severity'] ?? 'INFO';
    final timestamp = event['timestamp'] ?? '';
    final battery = event['battery'] ?? {};
    final panel = event['panel'] ?? {};
    final aiAlert = event['ai_alert'] ?? 'No alert message';

    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [Color(0xFF1E88E5), Color(0xFF1565C0), Color(0xFFF5F7FA)],
            stops: [0.0, 0.2, 0.2],
          ),
        ),
        child: SafeArea(
          child: Column(
            children: [
              _buildDetailHeader(context),
              Expanded(
                child: Container(
                  decoration: const BoxDecoration(
                    color: Color(0xFFF5F7FA),
                    borderRadius: BorderRadius.only(
                      topLeft: Radius.circular(32),
                      topRight: Radius.circular(32),
                    ),
                  ),
                  child: ListView(
                    padding: const EdgeInsets.all(24),
                    children: [
                      // Event Header Card
                      Container(
                        padding: const EdgeInsets.all(24),
                        decoration: BoxDecoration(
                          color: Colors.white,
                          borderRadius: BorderRadius.circular(20),
                          boxShadow: [
                            BoxShadow(
                              color: Colors.black.withOpacity(0.06),
                              blurRadius: 12,
                              offset: const Offset(0, 4),
                            ),
                          ],
                        ),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Row(
                              children: [
                                Container(
                                  padding: const EdgeInsets.symmetric(
                                    horizontal: 16,
                                    vertical: 8,
                                  ),
                                  decoration: BoxDecoration(
                                    color: _getSeverityColor(severity),
                                    borderRadius: BorderRadius.circular(10),
                                  ),
                                  child: Text(
                                    severity,
                                    style: const TextStyle(
                                      color: Colors.white,
                                      fontWeight: FontWeight.bold,
                                      fontSize: 13,
                                    ),
                                  ),
                                ),
                              ],
                            ),
                            const SizedBox(height: 16),
                            Text(
                              eventType.replaceAll('_', ' '),
                              style: const TextStyle(
                                fontSize: 24,
                                fontWeight: FontWeight.bold,
                                color: Color(0xFF2C3E50),
                              ),
                            ),
                            const SizedBox(height: 8),
                            Row(
                              children: [
                                Icon(
                                  Icons.access_time_rounded,
                                  size: 16,
                                  color: Colors.grey.shade600,
                                ),
                                const SizedBox(width: 6),
                                Text(
                                  _formatFullTimestamp(timestamp),
                                  style: TextStyle(
                                    fontSize: 14,
                                    color: Colors.grey.shade600,
                                  ),
                                ),
                              ],
                            ),
                          ],
                        ),
                      ),
                      const SizedBox(height: 20),

                      // Battery Status
                      _buildModernSection(
                        '🔋 Battery Status',
                        [
                          _buildDetailRow('Health', '${battery['health']?.toStringAsFixed(1) ?? '?'}%'),
                          _buildDetailRow('Voltage', '${battery['voltage']?.toStringAsFixed(2) ?? '?'}V'),
                          _buildDetailRow('Current', '${battery['current']?.toStringAsFixed(2) ?? '?'}A'),
                          _buildDetailRow('Temperature', '${battery['temperature']?.toString() ?? '?'}°C'),
                          _buildDetailRow('Cycle Count', battery['cycle_count']?.toString() ?? '?'),
                          if (battery['capacity_fade'] != null)
                            _buildDetailRow('Capacity Fade', '${battery['capacity_fade']?.toStringAsFixed(1)}%'),
                        ],
                      ),

                      // Panel Status
                      _buildModernSection(
                        '☀️ Panel Status',
                        [
                          _buildDetailRow('Health', '${panel['health']?.toString() ?? '?'}%'),
                          _buildDetailRow('Dust Areas', panel['dust_count']?.toString() ?? '0'),
                          _buildDetailRow('Cracks', panel['crack_count']?.toString() ?? '0'),
                          _buildDetailRow('Coverage', panel['cover_count']?.toString() ?? '0'),
                        ],
                      ),

                      // AI Alert
                      _buildModernSection(
                        '💬 AI Alert',
                        [
                          Container(
                            padding: const EdgeInsets.all(18),
                            decoration: BoxDecoration(
                              gradient: const LinearGradient(
                                colors: [Color(0xFFFFF3E0), Color(0xFFFFE0B2)],
                              ),
                              borderRadius: BorderRadius.circular(14),
                              border: Border.all(
                                color: const Color(0xFFFFB74D).withOpacity(0.3),
                                width: 1.5,
                              ),
                            ),
                            child: Row(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                const Icon(
                                  Icons.info_outline_rounded,
                                  color: Color(0xFFFF8A00),
                                  size: 24,
                                ),
                                const SizedBox(width: 12),
                                Expanded(
                                  child: Text(
                                    aiAlert,
                                    style: const TextStyle(
                                      fontSize: 14,
                                      height: 1.5,
                                      color: Color(0xFF5D4037),
                                    ),
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildDetailHeader(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(20),
      child: Row(
        children: [
          Container(
            decoration: BoxDecoration(
              color: Colors.white.withOpacity(0.2),
              borderRadius: BorderRadius.circular(12),
            ),
            child: IconButton(
              icon: const Icon(Icons.arrow_back_rounded, color: Colors.white),
              onPressed: () => Navigator.pop(context),
            ),
          ),
          const SizedBox(width: 16),
          const Text(
            'Event Details',
            style: TextStyle(
              color: Colors.white,
              fontSize: 22,
              fontWeight: FontWeight.bold,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildModernSection(String title, List<Widget> children) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          title,
          style: const TextStyle(
            fontSize: 18,
            fontWeight: FontWeight.bold,
            color: Color(0xFF2C3E50),
          ),
        ),
        const SizedBox(height: 12),
        Container(
          padding: const EdgeInsets.all(20),
          decoration: BoxDecoration(
            color: Colors.white,
            borderRadius: BorderRadius.circular(18),
            boxShadow: [
              BoxShadow(
                color: Colors.black.withOpacity(0.04),
                blurRadius: 10,
                offset: const Offset(0, 4),
              ),
            ],
          ),
          child: Column(children: children),
        ),
        const SizedBox(height: 20),
      ],
    );
  }

  Widget _buildDetailRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 10),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: TextStyle(
              fontSize: 15,
              color: Colors.grey.shade700,
            ),
          ),
          Text(
            value,
            style: const TextStyle(
              fontSize: 15,
              fontWeight: FontWeight.bold,
              color: Color(0xFF2C3E50),
            ),
          ),
        ],
      ),
    );
  }
}
