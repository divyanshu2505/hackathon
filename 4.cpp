#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <cstdlib>
#include <sqlite3.h>
#include <thread>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

// Database helper class
class DatabaseHelper {
private:
    sqlite3* db;
    
public:
    DatabaseHelper() {
        if (sqlite3_open("elderly_care.db", &db) != SQLITE_OK) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        } else {
            initializeDatabase();
        }
    }
    
    ~DatabaseHelper() {
        sqlite3_close(db);
    }
    
    void initializeDatabase() {
        const char* elderlyTable = "CREATE TABLE IF NOT EXISTS elderly_profiles ("
                                  "user_id TEXT PRIMARY KEY,"
                                  "name TEXT,"
                                  "age INTEGER,"
                                  "address TEXT,"
                                  "emergency_contacts TEXT,"
                                  "medical_conditions TEXT,"
                                  "medication_schedule TEXT,"
                                  "daily_routines TEXT);";
        
        const char* healthTable = "CREATE TABLE IF NOT EXISTS health_data ("
                                 "record_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                 "user_id TEXT,"
                                 "timestamp DATETIME,"
                                 "heart_rate INTEGER,"
                                 "blood_pressure TEXT,"
                                 "blood_glucose REAL,"
                                 "oxygen_level INTEGER,"
                                 "FOREIGN KEY (user_id) REFERENCES elderly_profiles (user_id));";
        
        const char* activityTable = "CREATE TABLE IF NOT EXISTS activity_data ("
                                   "record_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                   "user_id TEXT,"
                                   "timestamp DATETIME,"
                                   "activity_type TEXT,"
                                   "duration INTEGER,"
                                   "FOREIGN KEY (user_id) REFERENCES elderly_profiles (user_id));";
        
        const char* alertsTable = "CREATE TABLE IF NOT EXISTS alerts ("
                                 "alert_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                 "user_id TEXT,"
                                 "timestamp DATETIME,"
                                 "alert_type TEXT,"
                                 "severity TEXT,"
                                 "message TEXT,"
                                 "status TEXT DEFAULT 'pending',"
                                 "FOREIGN KEY (user_id) REFERENCES elderly_profiles (user_id));";
        
        const char* remindersTable = "CREATE TABLE IF NOT EXISTS reminders ("
                                    "reminder_id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                    "user_id TEXT,"
                                    "timestamp DATETIME,"
                                    "reminder_type TEXT,"
                                    "message TEXT,"
                                    "status TEXT DEFAULT 'pending',"
                                    "FOREIGN KEY (user_id) REFERENCES elderly_profiles (user_id));";
        
        executeQuery(elderlyTable);
        executeQuery(healthTable);
        executeQuery(activityTable);
        executeQuery(alertsTable);
        executeQuery(remindersTable);
    }
    
    bool executeQuery(const char* query) {
        char* errMsg = 0;
        if (sqlite3_exec(db, query, 0, 0, &errMsg) != SQLITE_OK) {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }
    
    sqlite3* getDB() { return db; }
};

// Data structures
struct HealthData {
    std::string userId;
    std::string timestamp;
    int heartRate;
    std::string bloodPressure;
    double bloodGlucose;
    int oxygenLevel;
};

struct ActivityData {
    std::string userId;
    std::string timestamp;
    std::string activityType;
    int duration;
};

struct Alert {
    int alertId;
    std::string userId;
    std::string timestamp;
    std::string alertType;
    std::string severity;
    std::string message;
    std::string status;
};

struct Reminder {
    std::string userId;
    std::string timestamp;
    std::string reminderType;
    std::string message;
};

// Helper functions
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Health Monitoring Agent
class HealthMonitoringAgent {
private:
    DatabaseHelper dbHelper;
    std::default_random_engine generator;
    
public:
    HealthMonitoringAgent() : generator(std::chrono::system_clock::now().time_since_epoch().count()) {}
    
    void recordHealthData(const std::string& userId, int heartRate, const std::string& bloodPressure, 
                         double bloodGlucose, int oxygenLevel) {
        sqlite3_stmt* stmt;
        const char* query = "INSERT INTO health_data (user_id, timestamp, heart_rate, blood_pressure, "
                           "blood_glucose, oxygen_level) VALUES (?, ?, ?, ?, ?, ?)";
        
        std::string timestamp = getCurrentTimestamp();
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, timestamp.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 3, heartRate);
            sqlite3_bind_text(stmt, 4, bloodPressure.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 5, bloodGlucose);
            sqlite3_bind_int(stmt, 6, oxygenLevel);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            
            // Check for critical values and generate alerts
            std::vector<std::tuple<std::string, std::string, std::string, std::string, std::string>> alerts;
            
            if (heartRate < 50 || heartRate > 120) {
                alerts.emplace_back(
                    userId, timestamp, "health_alert", "high",
                    "Abnormal heart rate: " + std::to_string(heartRate) + " bpm"
                );
            }
            
            if (bloodGlucose < 70 || bloodGlucose > 180) {
                alerts.emplace_back(
                    userId, timestamp, "health_alert", "high",
                    "Abnormal blood glucose: " + std::to_string(bloodGlucose) + " mg/dL"
                );
            }
            
            if (oxygenLevel < 90) {
                alerts.emplace_back(
                    userId, timestamp, "health_alert", "critical",
                    "Low oxygen level: " + std::to_string(oxygenLevel) + "%"
                );
            }
            
            // Insert alerts
            if (!alerts.empty()) {
                const char* alertQuery = "INSERT INTO alerts (user_id, timestamp, alert_type, severity, message) "
                                        "VALUES (?, ?, ?, ?, ?)";
                
                for (const auto& alert : alerts) {
                    sqlite3_stmt* alertStmt;
                    if (sqlite3_prepare_v2(dbHelper.getDB(), alertQuery, -1, &alertStmt, 0) == SQLITE_OK) {
                        sqlite3_bind_text(alertStmt, 1, std::get<0>(alert).c_str(), -1, SQLITE_STATIC);
                        sqlite3_bind_text(alertStmt, 2, std::get<1>(alert).c_str(), -1, SQLITE_STATIC);
                        sqlite3_bind_text(alertStmt, 3, std::get<2>(alert).c_str(), -1, SQLITE_STATIC);
                        sqlite3_bind_text(alertStmt, 4, std::get<3>(alert).c_str(), -1, SQLITE_STATIC);
                        sqlite3_bind_text(alertStmt, 5, std::get<4>(alert).c_str(), -1, SQLITE_STATIC);
                        
                        sqlite3_step(alertStmt);
                        sqlite3_finalize(alertStmt);
                    }
                }
            }
        }
    }
};

// Safety Monitoring Agent
class SafetyMonitoringAgent {
private:
    DatabaseHelper dbHelper;
    std::default_random_engine generator;
    
public:
    SafetyMonitoringAgent() : generator(std::chrono::system_clock::now().time_since_epoch().count()) {}
    
    void recordActivity(const std::string& userId, const std::string& activityType, int duration) {
        sqlite3_stmt* stmt;
        const char* query = "INSERT INTO activity_data (user_id, timestamp, activity_type, duration) "
                           "VALUES (?, ?, ?, ?)";
        
        std::string timestamp = getCurrentTimestamp();
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, timestamp.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, activityType.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 4, duration);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            
            // Check for safety concerns
            if (activityType == "fall") {
                createAlert(userId, timestamp, "safety_alert", "critical", "Fall detected!");
            } else if (activityType == "movement" && duration > 3600) {
                createAlert(userId, timestamp, "safety_alert", "high", 
                          "No movement detected for an extended period");
            }
        }
    }
    
private:
    void createAlert(const std::string& userId, const std::string& timestamp, 
                    const std::string& alertType, const std::string& severity, 
                    const std::string& message) {
        sqlite3_stmt* stmt;
        const char* query = "INSERT INTO alerts (user_id, timestamp, alert_type, severity, message) "
                           "VALUES (?, ?, ?, ?, ?)";
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, timestamp.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, alertType.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, severity.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 5, message.c_str(), -1, SQLITE_STATIC);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
};

// Reminder Agent
class ReminderAgent {
private:
    DatabaseHelper dbHelper;
    std::map<std::string, std::string> activeReminders;
    
public:
    void checkAndTriggerReminders(const std::string& userId) {
        // In a real implementation, this would check the medication schedule
        // and daily routines from the database and trigger reminders
        
        // For this simplified version, we'll just demonstrate the concept
        std::string now = getCurrentTimestamp();
        
        // Example: Trigger a medication reminder at specific times
        std::string currentTime = now.substr(11, 5); // Get HH:MM
        
        if (currentTime == "08:00") {
            triggerReminder(userId, "medication", "Time to take morning medication");
        } else if (currentTime == "12:30") {
            triggerReminder(userId, "activity", "Time for lunch");
        } else if (currentTime == "20:00") {
            triggerReminder(userId, "medication", "Time to take evening medication");
        }
    }
    
    void triggerReminder(const std::string& userId, const std::string& reminderType, 
                        const std::string& message) {
        std::string key = userId + "_" + reminderType + "_" + message;
        
        if (activeReminders.find(key) == activeReminders.end() || 
            activeReminders[key] != getCurrentTimestamp().substr(0, 10)) {
            
            sqlite3_stmt* stmt;
            const char* query = "INSERT INTO reminders (user_id, timestamp, reminder_type, message) "
                               "VALUES (?, ?, ?, ?)";
            
            std::string timestamp = getCurrentTimestamp();
            
            if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, timestamp.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, reminderType.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 4, message.c_str(), -1, SQLITE_STATIC);
                
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
                
                activeReminders[key] = timestamp.substr(0, 10);
                
                std::cout << "\nREMINDER for " << userId << ": " << message << std::endl;
            }
        }
    }
    
    void scheduleReminders(const std::string& userId, int intervalMinutes = 1) {
        while (true) {
            checkAndTriggerReminders(userId);
            std::this_thread::sleep_for(std::chrono::minutes(intervalMinutes));
        }
    }
};

// Caregiver Coordinator Agent
class CaregiverCoordinatorAgent {
private:
    DatabaseHelper dbHelper;
    
public:
    std::vector<Alert> getPendingAlerts(const std::string& userId = "") {
        std::vector<Alert> alerts;
        sqlite3_stmt* stmt;
        const char* query;
        
        if (userId.empty()) {
            query = "SELECT * FROM alerts WHERE status = 'pending' ORDER BY timestamp DESC";
        } else {
            query = "SELECT * FROM alerts WHERE status = 'pending' AND user_id = ? ORDER BY timestamp DESC";
        }
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            if (!userId.empty()) {
                sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_STATIC);
            }
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                Alert alert;
                alert.alertId = sqlite3_column_int(stmt, 0);
                alert.userId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                alert.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                alert.alertType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                alert.severity = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
                alert.message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
                alert.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
                
                alerts.push_back(alert);
            }
        }
        
        sqlite3_finalize(stmt);
        return alerts;
    }
    
    void updateAlertStatus(int alertId, const std::string& status, const std::string& notes = "") {
        sqlite3_stmt* stmt;
        const char* query = "UPDATE alerts SET status = ?, notes = ? WHERE alert_id = ?";
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, notes.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 3, alertId);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
};

// Main system class
class ElderlyCareSystem {
private:
    HealthMonitoringAgent healthAgent;
    SafetyMonitoringAgent safetyAgent;
    ReminderAgent reminderAgent;
    CaregiverCoordinatorAgent caregiverAgent;
    
public:
    void addSampleData() {
        DatabaseHelper db;
        
        // Add sample elderly profile
        const char* insertProfile = "INSERT OR IGNORE INTO elderly_profiles "
                                  "(user_id, name, age, address, emergency_contacts, "
                                  "medical_conditions, medication_schedule, daily_routines) "
                                  "VALUES ('ELD001', 'John Smith', 78, '123 Maple St', "
                                  "'Mary Smith (daughter): +1234567890', "
                                  "'Type 2 Diabetes, Hypertension', "
                                  "'[{\"name\":\"Metformin\",\"dosage\":\"500mg\",\"times\":[\"08:00\",\"20:00\"]}]', "
                                  "'[{\"activity\":\"Morning walk\",\"time\":\"07:00\"}]')";
        
        db.executeQuery(insertProfile);
        
        // Add sample health data
        const char* insertHealth = "INSERT OR IGNORE INTO health_data "
                                  "(user_id, timestamp, heart_rate, blood_pressure, "
                                  "blood_glucose, oxygen_level) VALUES "
                                  "('ELD001', '2023-06-01 08:00:00', 72, '120/80', 110, 98), "
                                  "('ELD001', '2023-06-01 12:00:00', 75, '125/82', 115, 97)";
        
        db.executeQuery(insertHealth);
        
        // Add sample activity data
        const char* insertActivity = "INSERT OR IGNORE INTO activity_data "
                                    "(user_id, timestamp, activity_type, duration) VALUES "
                                    "('ELD001', '2023-06-01 07:00:00', 'walk', 1800), "
                                    "('ELD001', '2023-06-01 09:30:00', 'movement', 300)";
        
        db.executeQuery(insertActivity);
    }
    
    void simulateRealTimeMonitoring(const std::string& userId = "ELD001", int durationHours = 1) {
        std::cout << "\nStarting real-time monitoring simulation for " << durationHours << " hour(s)...\n";
        
        // Start reminder scheduler in background
        std::thread reminderThread(&ReminderAgent::scheduleReminders, &reminderAgent, userId);
        reminderThread.detach();
        
        auto endTime = std::chrono::system_clock::now() + std::chrono::hours(durationHours);
        std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
        
        while (std::chrono::system_clock::now() < endTime) {
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            tm* localTime = std::localtime(&now);
            
            // Simulate health data (every 15 minutes)
            if (localTime->tm_min % 15 == 0) {
                std::uniform_int_distribution<int> hrDist(60, 100);
                std::uniform_int_distribution<int> bpSysDist(110, 135);
                std::uniform_int_distribution<int> bpDiaDist(70, 85);
                std::uniform_int_distribution<int> glucoseDist(80, 140);
                std::uniform_int_distribution<int> oxygenDist(95, 99);
                
                int hr = hrDist(generator);
                std::string bp = std::to_string(bpSysDist(generator)) + "/" + std::to_string(bpDiaDist(generator));
                double glucose = glucoseDist(generator);
                int oxygen = oxygenDist(generator);
                
                // Occasionally simulate anomalies
                if (std::rand() % 10 == 0) {
                    hr = (std::rand() % 2 == 0) ? std::rand() % 15 + 40 : std::rand() % 20 + 110;
                    glucose = (std::rand() % 2 == 0) ? std::rand() % 20 + 50 : std::rand() % 50 + 150;
                    if (std::rand() % 3 == 0) oxygen = std::rand() % 10 + 85;
                }
                
                healthAgent.recordHealthData(userId, hr, bp, glucose, oxygen);
                std::cout << "\nRecorded health data - HR: " << hr << ", BP: " << bp 
                          << ", Glucose: " << glucose << ", O2: " << oxygen << "%\n";
            }
            
            // Simulate activity data (every 5 minutes)
            if (localTime->tm_min % 5 == 0) {
                std::vector<std::string> activities = {"movement", "movement", "movement", "movement",
                                                      "movement", "movement", "movement", "movement",
                                                      "rest", "rest", "fall"};
                std::string activity = activities[std::rand() % activities.size()];
                int duration = (activity == "movement") ? std::rand() % 570 + 30 : std::rand() % 3300 + 300;
                
                safetyAgent.recordActivity(userId, activity, duration);
                std::cout << "\nRecorded activity - " << activity << " for " << duration << " seconds\n";
                
                // Check for alerts
                auto alerts = caregiverAgent.getPendingAlerts(userId);
                if (!alerts.empty()) {
                    std::cout << "\n*** ALERTS GENERATED ***\n";
                    for (const auto& alert : alerts) {
                        std::cout << "[" << alert.severity << "] " << alert.message 
                                  << " (at " << alert.timestamp << ")\n";
                        // Mark as addressed
                        caregiverAgent.updateAlertStatus(alert.alertId, "addressed", "Simulated response");
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(60)); // Wait 1 minute between checks
        }
    }
    
    void runMenu() {
        std::cout << "Elderly Care AI System - Demonstration\n";
        std::cout << "-------------------------------------\n";
        std::cout << "1. Simulate real-time monitoring\n";
        std::cout << "2. View pending alerts\n";
        std::cout << "3. Exit\n";
        
        while (true) {
            std::cout << "\nSelect an option (1-3): ";
            std::string choice;
            std::getline(std::cin, choice);
            
            if (choice == "1") {
                std::cout << "Enter duration in hours (1-24): ";
                int duration;
                std::cin >> duration;
                std::cin.ignore();
                simulateRealTimeMonitoring("ELD001", duration);
            } else if (choice == "2") {
                auto alerts = caregiverAgent.getPendingAlerts();
                if (alerts.empty()) {
                    std::cout << "\nNo pending alerts\n";
                } else {
                    std::cout << "\nPending Alerts:\n";
                    for (const auto& alert : alerts) {
                        std::cout << "\n[" << alert.severity << "] " << alert.message << "\n";
                        std::cout << "Type: " << alert.alertType << " | Time: " << alert.timestamp << "\n";
                    }
                }
            } else if (choice == "3") {
                break;
            } else {
                std::cout << "Invalid choice, please try again\n";
            }
        }
    }
};

int main() {
    ElderlyCareSystem system;
    system.addSampleData();
    system.runMenu();
    return 0;
}