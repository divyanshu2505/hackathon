#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <cstdlib>
#include <sqlite3.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <openssl/md5.h>

// Database helper class
class DatabaseHelper {
private:
    sqlite3* db;
    
public:
    DatabaseHelper() {
        if (sqlite3_open("recruitment.db", &db) != SQLITE_OK) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        } else {
            initializeDatabase();
        }
    }
    
    ~DatabaseHelper() {
        sqlite3_close(db);
    }
    
    void initializeDatabase() {
        const char* jobDescTable = "CREATE TABLE IF NOT EXISTS job_descriptions ("
                                  "jd_id TEXT PRIMARY KEY,"
                                  "title TEXT,"
                                  "raw_text TEXT,"
                                  "summary TEXT,"
                                  "required_skills TEXT,"
                                  "required_experience TEXT,"
                                  "qualifications TEXT,"
                                  "responsibilities TEXT,"
                                  "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);";
        
        const char* candidatesTable = "CREATE TABLE IF NOT EXISTS candidates ("
                                    "candidate_id TEXT PRIMARY KEY,"
                                    "name TEXT,"
                                    "email TEXT,"
                                    "phone TEXT,"
                                    "raw_cv TEXT,"
                                    "summary TEXT,"
                                    "skills TEXT,"
                                    "experience TEXT,"
                                    "education TEXT,"
                                    "certifications TEXT,"
                                    "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);";
        
        const char* matchesTable = "CREATE TABLE IF NOT EXISTS matches ("
                                 "match_id TEXT PRIMARY KEY,"
                                 "jd_id TEXT,"
                                 "candidate_id TEXT,"
                                 "skills_match REAL,"
                                 "experience_match REAL,"
                                 "qualifications_match REAL,"
                                 "overall_match REAL,"
                                 "status TEXT DEFAULT 'pending',"
                                 "FOREIGN KEY (jd_id) REFERENCES job_descriptions (jd_id),"
                                 "FOREIGN KEY (candidate_id) REFERENCES candidates (candidate_id));";
        
        const char* interviewsTable = "CREATE TABLE IF NOT EXISTS interviews ("
                                    "interview_id TEXT PRIMARY KEY,"
                                    "match_id TEXT,"
                                    "scheduled_time DATETIME,"
                                    "duration INTEGER,"
                                    "format TEXT,"
                                    "status TEXT DEFAULT 'scheduled',"
                                    "reminder_sent BOOLEAN DEFAULT 0,"
                                    "FOREIGN KEY (match_id) REFERENCES matches (match_id));";
        
        executeQuery(jobDescTable);
        executeQuery(candidatesTable);
        executeQuery(matchesTable);
        executeQuery(interviewsTable);
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

// Helper functions
std::string generateId(const std::string& prefix, const std::string& input) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((const unsigned char*)input.c_str(), input.size(), digest);
    
    char mdString[33];
    for (int i = 0; i < 16; i++)
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
    
    return prefix + "-" + std::string(mdString).substr(0, 8);
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Data structures
struct JobDescription {
    std::string jd_id;
    std::string title;
    std::string raw_text;
    std::string summary;
    std::vector<std::string> required_skills;
    std::string required_experience;
    std::vector<std::string> qualifications;
    std::vector<std::string> responsibilities;
};

struct Candidate {
    std::string candidate_id;
    std::string name;
    std::string email;
    std::string phone;
    std::string raw_cv;
    std::string summary;
    std::vector<std::string> skills;
    std::vector<std::map<std::string, std::string>> experience;
    std::vector<std::map<std::string, std::string>> education;
    std::vector<std::string> certifications;
};

struct MatchResult {
    std::string match_id;
    std::string jd_id;
    std::string candidate_id;
    double skills_match;
    double experience_match;
    double qualifications_match;
    double overall_match;
    std::string status;
};

struct Interview {
    std::string interview_id;
    std::string match_id;
    std::string scheduled_time;
    int duration;
    std::string format;
    std::string status;
};

// JD Processing Agent
class JDProcessingAgent {
private:
    DatabaseHelper dbHelper;
    
public:
    JobDescription processAndStoreJD(const std::string& title, const std::string& jdText) {
        JobDescription jd;
        jd.jd_id = generateId("JD", jdText);
        jd.title = title;
        jd.raw_text = jdText;
        
        // Simplified processing (in real system would use NLP)
        jd.summary = "Looking for " + title + " with relevant skills";
        
        // Extract skills (simplified)
        std::vector<std::string> commonSkills = {"Python", "Java", "SQL", "AWS", "Docker"};
        for (const auto& skill : commonSkills) {
            if (jdText.find(skill) != std::string::npos) {
                jd.required_skills.push_back(skill);
            }
        }
        
        // Extract experience (simplified)
        size_t expPos = jdText.find("experience");
        if (expPos != std::string::npos) {
            jd.required_experience = jdText.substr(expPos, 20); // Get next 20 chars
        } else {
            jd.required_experience = "Not specified";
        }
        
        // Extract qualifications (simplified)
        if (jdText.find("Bachelor") != std::string::npos) {
            jd.qualifications.push_back("Bachelor's degree");
        }
        if (jdText.find("Master") != std::string::npos) {
            jd.qualifications.push_back("Master's degree");
        }
        
        // Extract responsibilities (simplified)
        size_t respPos = jdText.find("Responsibilities");
        if (respPos != std::string::npos) {
            jd.responsibilities.push_back(jdText.substr(respPos, 100)); // Get next 100 chars
        }
        
        // Store in database
        sqlite3_stmt* stmt;
        const char* query = "INSERT INTO job_descriptions (jd_id, title, raw_text, summary, "
                           "required_skills, required_experience, qualifications, responsibilities) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            // Convert vectors to JSON strings
            std::string skillsStr = "[" + joinStrings(jd.required_skills) + "]";
            std::string qualsStr = "[" + joinStrings(jd.qualifications) + "]";
            std::string respStr = "[" + joinStrings(jd.responsibilities) + "]";
            
            sqlite3_bind_text(stmt, 1, jd.jd_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, jd.title.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, jd.raw_text.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, jd.summary.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 5, skillsStr.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 6, jd.required_experience.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 7, qualsStr.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 8, respStr.c_str(), -1, SQLITE_STATIC);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        return jd;
    }
    
private:
    std::string joinStrings(const std::vector<std::string>& strings) {
        std::string result;
        for (size_t i = 0; i < strings.size(); ++i) {
            if (i != 0) result += ", ";
            result += "\"" + strings[i] + "\"";
        }
        return result;
    }
};

// CV Processing Agent
class CVProcessingAgent {
private:
    DatabaseHelper dbHelper;
    
public:
    Candidate processAndStoreCandidate(const std::string& name, const std::string& email, 
                                     const std::string& cvText, const std::string& phone = "") {
        Candidate candidate;
        candidate.candidate_id = generateId("CAND", name + email);
        candidate.name = name;
        candidate.email = email;
        candidate.phone = phone;
        candidate.raw_cv = cvText;
        
        // Simplified processing (in real system would use NLP)
        candidate.summary = "Experienced " + extractTitleFromCV(cvText);
        
        // Extract skills (simplified)
        std::vector<std::string> commonSkills = {"Python", "Java", "SQL", "AWS", "Docker"};
        for (const auto& skill : commonSkills) {
            if (cvText.find(skill) != std::string::npos) {
                candidate.skills.push_back(skill);
            }
        }
        
        // Extract experience (simplified)
        std::map<std::string, std::string> expEntry;
        expEntry["title"] = extractTitleFromCV(cvText);
        expEntry["company"] = "Sample Company";
        expEntry["from"] = "2018";
        expEntry["to"] = "present";
        candidate.experience.push_back(expEntry);
        
        // Extract education (simplified)
        std::map<std::string, std::string> eduEntry;
        if (cvText.find("Bachelor") != std::string::npos) {
            eduEntry["degree"] = "Bachelor's degree";
            eduEntry["field"] = "Computer Science";
            eduEntry["institution"] = "University";
            candidate.education.push_back(eduEntry);
        }
        
        // Extract certifications (simplified)
        if (cvText.find("AWS") != std::string::npos) {
            candidate.certifications.push_back("AWS Certified");
        }
        
        // Store in database
        sqlite3_stmt* stmt;
        const char* query = "INSERT INTO candidates (candidate_id, name, email, phone, raw_cv, "
                           "summary, skills, experience, education, certifications) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            // Convert vectors to JSON strings
            std::string skillsStr = "[" + joinStrings(candidate.skills) + "]";
            std::string expStr = "[{\"title\":\"" + expEntry["title"] + "\",\"company\":\"" + 
                               expEntry["company"] + "\",\"from\":\"" + expEntry["from"] + 
                               "\",\"to\":\"" + expEntry["to"] + "\"}]";
            std::string eduStr = "[{\"degree\":\"" + eduEntry["degree"] + "\",\"field\":\"" + 
                               eduEntry["field"] + "\",\"institution\":\"" + eduEntry["institution"] + "\"}]";
            std::string certsStr = "[" + joinStrings(candidate.certifications) + "]";
            
            sqlite3_bind_text(stmt, 1, candidate.candidate_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, candidate.name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, candidate.email.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, candidate.phone.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 5, candidate.raw_cv.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 6, candidate.summary.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 7, skillsStr.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 8, expStr.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 9, eduStr.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 10, certsStr.c_str(), -1, SQLITE_STATIC);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        return candidate;
    }
    
private:
    std::string extractTitleFromCV(const std::string& cvText) {
        size_t pos = cvText.find('\n');
        if (pos != std::string::npos) {
            std::string firstLine = cvText.substr(0, pos);
            if (firstLine.find("Senior") != std::string::npos) return "Senior Developer";
            if (firstLine.find("Junior") != std::string::npos) return "Junior Developer";
            if (firstLine.find("Developer") != std::string::npos) return "Developer";
        }
        return "Professional";
    }
    
    std::string joinStrings(const std::vector<std::string>& strings) {
        std::string result;
        for (size_t i = 0; i < strings.size(); ++i) {
            if (i != 0) result += ", ";
            result += "\"" + strings[i] + "\"";
        }
        return result;
    }
};

// Matching Agent
class MatchingAgent {
private:
    DatabaseHelper dbHelper;
    
public:
    MatchResult calculateMatch(const std::string& jdId, const std::string& candidateId) {
        MatchResult match;
        match.match_id = generateId("MATCH", jdId + candidateId);
        match.jd_id = jdId;
        match.candidate_id = candidateId;
        
        // Get JD data
        sqlite3_stmt* jdStmt;
        const char* jdQuery = "SELECT required_skills, required_experience, qualifications "
                             "FROM job_descriptions WHERE jd_id = ?";
        
        std::vector<std::string> jdSkills;
        std::string jdExperience;
        std::vector<std::string> jdQualifications;
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), jdQuery, -1, &jdStmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(jdStmt, 1, jdId.c_str(), -1, SQLITE_STATIC);
            
            if (sqlite3_step(jdStmt) == SQLITE_ROW) {
                jdSkills = parseJsonArray(reinterpret_cast<const char*>(sqlite3_column_text(jdStmt, 0)));
                jdExperience = reinterpret_cast<const char*>(sqlite3_column_text(jdStmt, 1));
                jdQualifications = parseJsonArray(reinterpret_cast<const char*>(sqlite3_column_text(jdStmt, 2)));
            }
        }
        sqlite3_finalize(jdStmt);
        
        // Get candidate data
        sqlite3_stmt* candStmt;
        const char* candQuery = "SELECT skills, experience, education FROM candidates WHERE candidate_id = ?";
        
        std::vector<std::string> candSkills;
        std::vector<std::map<std::string, std::string>> candExperience;
        std::vector<std::map<std::string, std::string>> candEducation;
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), candQuery, -1, &candStmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(candStmt, 1, candidateId.c_str(), -1, SQLITE_STATIC);
            
            if (sqlite3_step(candStmt) == SQLITE_ROW) {
                candSkills = parseJsonArray(reinterpret_cast<const char*>(sqlite3_column_text(candStmt, 0)));
                candExperience = parseJsonObjectArray(reinterpret_cast<const char*>(sqlite3_column_text(candStmt, 1)));
                candEducation = parseJsonObjectArray(reinterpret_cast<const char*>(sqlite3_column_text(candStmt, 2)));
            }
        }
        sqlite3_finalize(candStmt);
        
        // Calculate matches (simplified)
        match.skills_match = calculateSkillsMatch(jdSkills, candSkills);
        match.experience_match = calculateExperienceMatch(jdExperience, candExperience);
        match.qualifications_match = calculateQualificationsMatch(jdQualifications, candEducation);
        
        // Overall match (weighted average)
        match.overall_match = (match.skills_match * 0.4 + match.experience_match * 0.3 + 
                              match.qualifications_match * 0.3) * 100;
        match.status = "pending";
        
        // Store match result
        sqlite3_stmt* stmt;
        const char* query = "INSERT INTO matches (match_id, jd_id, candidate_id, skills_match, "
                           "experience_match, qualifications_match, overall_match) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?)";
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, match.match_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, match.jd_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, match.candidate_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 4, match.skills_match * 100);
            sqlite3_bind_double(stmt, 5, match.experience_match * 100);
            sqlite3_bind_double(stmt, 6, match.qualifications_match * 100);
            sqlite3_bind_double(stmt, 7, match.overall_match);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        return match;
    }
    
    std::vector<MatchResult> getTopMatches(const std::string& jdId, double threshold = 80.0, int limit = 10) {
        std::vector<MatchResult> matches;
        
        sqlite3_stmt* stmt;
        const char* query = "SELECT * FROM matches WHERE jd_id = ? AND overall_match >= ? "
                           "ORDER BY overall_match DESC LIMIT ?";
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, jdId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 2, threshold);
            sqlite3_bind_int(stmt, 3, limit);
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                MatchResult match;
                match.match_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                match.jd_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                match.candidate_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                match.skills_match = sqlite3_column_double(stmt, 3) / 100;
                match.experience_match = sqlite3_column_double(stmt, 4) / 100;
                match.qualifications_match = sqlite3_column_double(stmt, 5) / 100;
                match.overall_match = sqlite3_column_double(stmt, 6);
                match.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
                
                matches.push_back(match);
            }
        }
        sqlite3_finalize(stmt);
        
        return matches;
    }
    
private:
    std::vector<std::string> parseJsonArray(const char* jsonStr) {
        std::vector<std::string> result;
        if (!jsonStr) return result;
        
        std::string str(jsonStr);
        size_t start = str.find('[');
        size_t end = str.find(']');
        
        if (start != std::string::npos && end != std::string::npos && end > start + 1) {
            std::string content = str.substr(start + 1, end - start - 1);
            size_t pos = 0;
            while ((pos = content.find('"')) != std::string::npos) {
                size_t endPos = content.find('"', pos + 1);
                if (endPos == std::string::npos) break;
                
                result.push_back(content.substr(pos + 1, endPos - pos - 1));
                content = content.substr(endPos + 1);
            }
        }
        
        return result;
    }
    
    std::vector<std::map<std::string, std::string>> parseJsonObjectArray(const char* jsonStr) {
        std::vector<std::map<std::string, std::string>> result;
        if (!jsonStr) return result;
        
        // Simplified parsing - in real system use a proper JSON parser
        std::string str(jsonStr);
        size_t start = str.find('{');
        while (start != std::string::npos) {
            size_t end = str.find('}', start);
            if (end == std::string::npos) break;
            
            std::string objStr = str.substr(start + 1, end - start - 1);
            std::map<std::string, std::string> obj;
            
            size_t keyPos = 0;
            while ((keyPos = objStr.find('"')) != std::string::npos) {
                size_t keyEnd = objStr.find('"', keyPos + 1);
                if (keyEnd == std::string::npos) break;
                
                std::string key = objStr.substr(keyPos + 1, keyEnd - keyPos - 1);
                
                size_t valPos = objStr.find(':', keyEnd);
                if (valPos == std::string::npos) break;
                
                size_t valStart = objStr.find('"', valPos + 1);
                if (valStart == std::string::npos) break;
                
                size_t valEnd = objStr.find('"', valStart + 1);
                if (valEnd == std::string::npos) break;
                
                std::string value = objStr.substr(valStart + 1, valEnd - valStart - 1);
                obj[key] = value;
                
                objStr = objStr.substr(valEnd + 1);
            }
            
            result.push_back(obj);
            start = str.find('{', end + 1);
        }
        
        return result;
    }
    
    double calculateSkillsMatch(const std::vector<std::string>& jdSkills, 
                              const std::vector<std::string>& candSkills) {
        if (jdSkills.empty()) return 0.0;
        
        int matches = 0;
        for (const auto& skill : jdSkills) {
            if (std::find(candSkills.begin(), candSkills.end(), skill) != candSkills.end()) {
                matches++;
            }
        }
        
        return static_cast<double>(matches) / jdSkills.size();
    }
    
    double calculateExperienceMatch(const std::string& jdExp, 
                                  const std::vector<std::map<std::string, std::string>>& candExp) {
        if (jdExp.empty()) return 0.5; // Neutral score if no JD experience specified
        
        // Simplified - just check if candidate has any experience
        return candExp.empty() ? 0.0 : 0.8;
    }
    
    double calculateQualificationsMatch(const std::vector<std::string>& jdQuals, 
                                      const std::vector<std::map<std::string, std::string>>& candEdu) {
        if (jdQuals.empty()) return 0.5; // Neutral score if no JD qualifications specified
        
        // Simplified - just check if candidate has any education
        return candEdu.empty() ? 0.0 : 0.7;
    }
};

// Interview Scheduler Agent
class InterviewSchedulerAgent {
private:
    DatabaseHelper dbHelper;
    
public:
    Interview scheduleInterview(const std::string& matchId, const std::string& scheduledTime,
                              int duration = 60, const std::string& format = "virtual") {
        Interview interview;
        interview.interview_id = generateId("INT", matchId + scheduledTime);
        interview.match_id = matchId;
        interview.scheduled_time = scheduledTime;
        interview.duration = duration;
        interview.format = format;
        interview.status = "scheduled";
        
        // Store interview
        sqlite3_stmt* stmt;
        const char* query = "INSERT INTO interviews (interview_id, match_id, scheduled_time, "
                           "duration, format) VALUES (?, ?, ?, ?, ?)";
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, interview.interview_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, interview.match_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, interview.scheduled_time.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 4, interview.duration);
            sqlite3_bind_text(stmt, 5, interview.format.c_str(), -1, SQLITE_STATIC);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        // Update match status
        const char* updateQuery = "UPDATE matches SET status = 'interview_scheduled' WHERE match_id = ?";
        if (sqlite3_prepare_v2(dbHelper.getDB(), updateQuery, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, matchId.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        return interview;
    }
};

// Recruitment Orchestrator
class RecruitmentOrchestrator {
private:
    JDProcessingAgent jdProcessor;
    CVProcessingAgent cvProcessor;
    MatchingAgent matcher;
    InterviewSchedulerAgent scheduler;
    
public:
    void addSampleData() {
        DatabaseHelper db;
        
        // Sample job description
        const char* jdQuery = "INSERT OR IGNORE INTO job_descriptions "
                             "(jd_id, title, raw_text, summary, required_skills, "
                             "required_experience, qualifications, responsibilities) "
                             "VALUES ('JD-SAMPLE1', 'Senior Python Developer', "
                             "'Sample JD text', 'Looking for Python developer', "
                             "'[\"Python\", \"Django\", \"SQL\"]', '5+ years', "
                             "'[\"Bachelor\\'s degree\"]', '[\"Develop software\"]')";
        
        db.executeQuery(jdQuery);
        
        // Sample candidates
        const char* candQuery1 = "INSERT OR IGNORE INTO candidates "
                               "(candidate_id, name, email, raw_cv, summary, skills, "
                               "experience, education) "
                               "VALUES ('CAND-SAMPLE1', 'Alice Smith', 'alice@example.com', "
                               "'Sample CV', 'Experienced developer', '[\"Python\", \"SQL\"]', "
                               "'[{\"title\":\"Developer\"}]', '[{\"degree\":\"Bachelor\\'s degree\"}]')";
        
        const char* candQuery2 = "INSERT OR IGNORE INTO candidates "
                               "(candidate_id, name, email, raw_cv, summary, skills) "
                               "VALUES ('CAND-SAMPLE2', 'Bob Johnson', 'bob@example.com', "
                               "'Sample CV', 'Junior developer', '[\"Python\"]')";
        
        db.executeQuery(candQuery1);
        db.executeQuery(candQuery2);
    }
    
    void runDemo() {
        std::cout << "AI Recruitment System - Demonstration\n";
        std::cout << "-----------------------------------\n";
        
        // Process sample job description
        std::string jdTitle = "Senior Python Developer";
        std::string jdText = "We need a Python developer with 5+ years experience...";
        
        std::cout << "\nProcessing job description...\n";
        JobDescription jd = jdProcessor.processAndStoreJD(jdTitle, jdText);
        std::cout << "Processed JD: " << jd.title << " (ID: " << jd.jd_id << ")\n";
        
        // Process sample candidates
        std::cout << "\nProcessing candidates...\n";
        
        std::vector<Candidate> candidates;
        candidates.push_back(cvProcessor.processAndStoreCandidate(
            "Alice Smith", "alice@example.com", "Alice Smith\nPython Developer\n5 years experience"));
        
        candidates.push_back(cvProcessor.processAndStoreCandidate(
            "Bob Johnson", "bob@example.com", "Bob Johnson\nJunior Developer\n1 year experience"));
        
        for (const auto& cand : candidates) {
            std::cout << "Processed candidate: " << cand.name << " (ID: " << cand.candidate_id << ")\n";
        }
        
        // Match candidates to JD
        std::cout << "\nMatching candidates to job description...\n";
        
        std::vector<MatchResult> matches;
        for (const auto& cand : candidates) {
            matches.push_back(matcher.calculateMatch(jd.jd_id, cand.candidate_id));
            std::cout << "Match for " << cand.name << ": " 
                      << matches.back().overall_match << "%\n";
        }
        
        // Get top matches and schedule interviews
        std::cout << "\nScheduling interviews for top matches...\n";
        
        auto topMatches = matcher.getTopMatches(jd.jd_id, 50.0);
        for (const auto& match : topMatches) {
            // Schedule for tomorrow at 10 AM
            auto now = std::chrono::system_clock::now();
            auto tomorrow = now + std::chrono::hours(24);
            auto in_time_t = std::chrono::system_clock::to_time_t(tomorrow);
            std::tm tm = *std::localtime(&in_time_t);
            tm.tm_hour = 10;
            tm.tm_min = 0;
            
            std::stringstream ss;
            ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            
            Interview interview = scheduler.scheduleInterview(
                match.match_id, ss.str(), 45, "virtual");
            
            std::cout << "Scheduled interview for candidate " << match.candidate_id 
                      << " at " << interview.scheduled_time << "\n";
        }
        
        std::cout << "\nDemo completed!\n";
    }
};

int main() {
    RecruitmentOrchestrator orchestrator;
    orchestrator.addSampleData();
    orchestrator.runDemo();
    return 0;
}