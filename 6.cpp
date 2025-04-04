#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sqlite3.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <openssl/md5.h>

// Database helper class
class DatabaseHelper {
private:
    sqlite3* db;
    
public:
    DatabaseHelper() {
        if (sqlite3_open("data_products.db", &db) != SQLITE_OK) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        } else {
            initializeDatabase();
        }
    }
    
    ~DatabaseHelper() {
        sqlite3_close(db);
    }
    
    void initializeDatabase() {
        const char* useCasesTable = "CREATE TABLE IF NOT EXISTS use_cases ("
                                   "use_case_id TEXT PRIMARY KEY,"
                                   "title TEXT,"
                                   "description TEXT,"
                                   "business_domain TEXT,"
                                   "stakeholders TEXT,"
                                   "requirements TEXT,"
                                   "status TEXT DEFAULT 'draft');";
        
        const char* sourceSystemsTable = "CREATE TABLE IF NOT EXISTS source_systems ("
                                       "system_id TEXT PRIMARY KEY,"
                                       "name TEXT,"
                                       "description TEXT,"
                                       "owner TEXT,"
                                       "data_domain TEXT,"
                                       "metadata TEXT);";
        
        const char* sourceAttributesTable = "CREATE TABLE IF NOT EXISTS source_attributes ("
                                          "attribute_id TEXT PRIMARY KEY,"
                                          "system_id TEXT,"
                                          "name TEXT,"
                                          "data_type TEXT,"
                                          "description TEXT,"
                                          "sample_values TEXT,"
                                          "sensitivity TEXT,"
                                          "FOREIGN KEY (system_id) REFERENCES source_systems (system_id));";
        
        const char* dataProductsTable = "CREATE TABLE IF NOT EXISTS data_products ("
                                      "product_id TEXT PRIMARY KEY,"
                                      "use_case_id TEXT,"
                                      "name TEXT,"
                                      "description TEXT,"
                                      "structure TEXT,"
                                      "status TEXT DEFAULT 'design',"
                                      "certification_status TEXT DEFAULT 'not_certified',"
                                      "FOREIGN KEY (use_case_id) REFERENCES use_cases (use_case_id));";
        
        const char* dataProductAttributesTable = "CREATE TABLE IF NOT EXISTS data_product_attributes ("
                                               "attribute_id TEXT PRIMARY KEY,"
                                               "product_id TEXT,"
                                               "name TEXT,"
                                               "data_type TEXT,"
                                               "description TEXT,"
                                               "is_key BOOLEAN,"
                                               "sensitivity TEXT,"
                                               "FOREIGN KEY (product_id) REFERENCES data_products (product_id));";
        
        const char* attributeMappingsTable = "CREATE TABLE IF NOT EXISTS attribute_mappings ("
                                           "mapping_id TEXT PRIMARY KEY,"
                                           "product_id TEXT,"
                                           "target_attribute_id TEXT,"
                                           "source_attribute_id TEXT,"
                                           "transformation TEXT,"
                                           "transformation_spec TEXT,"
                                           "FOREIGN KEY (product_id) REFERENCES data_products (product_id),"
                                           "FOREIGN KEY (target_attribute_id) REFERENCES data_product_attributes (attribute_id),"
                                           "FOREIGN KEY (source_attribute_id) REFERENCES source_attributes (attribute_id));";
        
        const char* certificationChecksTable = "CREATE TABLE IF NOT EXISTS certification_checks ("
                                             "check_id TEXT PRIMARY KEY,"
                                             "product_id TEXT,"
                                             "check_type TEXT,"
                                             "description TEXT,"
                                             "status TEXT,"
                                             "comments TEXT,"
                                             "FOREIGN KEY (product_id) REFERENCES data_products (product_id));";
        
        executeQuery(useCasesTable);
        executeQuery(sourceSystemsTable);
        executeQuery(sourceAttributesTable);
        executeQuery(dataProductsTable);
        executeQuery(dataProductAttributesTable);
        executeQuery(attributeMappingsTable);
        executeQuery(certificationChecksTable);
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
struct UseCase {
    std::string use_case_id;
    std::string title;
    std::string description;
    std::string business_domain;
    std::vector<std::string> stakeholders;
    std::map<std::string, std::string> requirements;
    std::string status;
};

struct SourceSystem {
    std::string system_id;
    std::string name;
    std::string description;
    std::string owner;
    std::string data_domain;
    std::map<std::string, std::string> metadata;
};

struct SourceAttribute {
    std::string attribute_id;
    std::string system_id;
    std::string name;
    std::string data_type;
    std::string description;
    std::vector<std::string> sample_values;
    std::string sensitivity;
};

struct DataProduct {
    std::string product_id;
    std::string use_case_id;
    std::string name;
    std::string description;
    std::map<std::string, std::string> structure;
    std::string status;
    std::string certification_status;
};

struct DataProductAttribute {
    std::string attribute_id;
    std::string product_id;
    std::string name;
    std::string data_type;
    std::string description;
    bool is_key;
    std::string sensitivity;
};

struct AttributeMapping {
    std::string mapping_id;
    std::string product_id;
    std::string target_attribute_id;
    std::string source_attribute_id;
    std::string transformation;
    std::string transformation_spec;
};

struct CertificationCheck {
    std::string check_id;
    std::string product_id;
    std::string check_type;
    std::string description;
    std::string status;
    std::string comments;
};

// Use Case Analyst Agent
class UseCaseAnalystAgent {
private:
    DatabaseHelper dbHelper;
    
public:
    std::string createUseCase(const UseCase& useCase) {
        sqlite3_stmt* stmt;
        const char* query = "INSERT INTO use_cases (use_case_id, title, description, "
                           "business_domain, stakeholders, requirements) "
                           "VALUES (?, ?, ?, ?, ?, ?)";
        
        // Convert vectors/maps to JSON strings
        std::string stakeholdersStr = "[" + joinStrings(useCase.stakeholders) + "]";
        std::string requirementsStr = mapToJson(useCase.requirements);
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, useCase.use_case_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, useCase.title.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, useCase.description.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, useCase.business_domain.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 5, stakeholdersStr.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 6, requirementsStr.c_str(), -1, SQLITE_STATIC);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        return useCase.use_case_id;
    }
    
    UseCase getUseCase(const std::string& useCaseId) {
        UseCase useCase;
        sqlite3_stmt* stmt;
        const char* query = "SELECT * FROM use_cases WHERE use_case_id = ?";
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, useCaseId.c_str(), -1, SQLITE_STATIC);
            
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                useCase.use_case_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                useCase.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                useCase.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                useCase.business_domain = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                
                // Parse JSON strings
                std::string stakeholdersJson = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
                useCase.stakeholders = parseJsonArray(stakeholdersJson);
                
                std::string requirementsJson = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
                useCase.requirements = parseJsonObject(requirementsJson);
                
                useCase.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            }
        }
        sqlite3_finalize(stmt);
        return useCase;
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
    
    std::string mapToJson(const std::map<std::string, std::string>& map) {
        std::string result = "{";
        bool first = true;
        for (const auto& pair : map) {
            if (!first) result += ", ";
            result += "\"" + pair.first + "\": \"" + pair.second + "\"";
            first = false;
        }
        result += "}";
        return result;
    }
    
    std::vector<std::string> parseJsonArray(const std::string& jsonStr) {
        std::vector<std::string> result;
        size_t start = jsonStr.find('[');
        size_t end = jsonStr.find(']');
        
        if (start != std::string::npos && end != std::string::npos && end > start + 1) {
            std::string content = jsonStr.substr(start + 1, end - start - 1);
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
    
    std::map<std::string, std::string> parseJsonObject(const std::string& jsonStr) {
        std::map<std::string, std::string> result;
        size_t start = jsonStr.find('{');
        size_t end = jsonStr.find('}');
        
        if (start != std::string::npos && end != std::string::npos && end > start + 1) {
            std::string content = jsonStr.substr(start + 1, end - start - 1);
            size_t pos = 0;
            while ((pos = content.find('"')) != std::string::npos) {
                size_t keyEnd = content.find('"', pos + 1);
                if (keyEnd == std::string::npos) break;
                
                std::string key = content.substr(pos + 1, keyEnd - pos - 1);
                
                size_t valStart = content.find(':', keyEnd);
                if (valStart == std::string::npos) break;
                
                size_t valQuote = content.find('"', valStart + 1);
                if (valQuote == std::string::npos) break;
                
                size_t valEnd = content.find('"', valQuote + 1);
                if (valEnd == std::string::npos) break;
                
                std::string value = content.substr(valQuote + 1, valEnd - valQuote - 1);
                result[key] = value;
                
                content = content.substr(valEnd + 1);
            }
        }
        
        return result;
    }
};

// Data Product Designer Agent
class DataProductDesignerAgent {
private:
    DatabaseHelper dbHelper;
    
public:
    DataProduct designDataProduct(const std::string& useCaseId) {
        UseCaseAnalystAgent analyst;
        UseCase useCase = analyst.getUseCase(useCaseId);
        if (useCase.use_case_id.empty()) return DataProduct();
        
        DataProduct product;
        product.product_id = generateId("DP", useCaseId);
        product.use_case_id = useCaseId;
        product.name = useCase.title + " Data Product";
        product.description = "Data product for " + useCase.title + " use case";
        
        // Determine structure based on description
        product.structure["type"] = (useCase.description.find("analysis") != std::string::npos) ? 
                                   "analytical" : "operational";
        product.structure["refresh_frequency"] = "daily";
        product.structure["retention_period"] = "365 days";
        
        // Store product
        sqlite3_stmt* stmt;
        const char* query = "INSERT INTO data_products (product_id, use_case_id, name, "
                           "description, structure) VALUES (?, ?, ?, ?, ?)";
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            std::string structureStr = mapToJson(product.structure);
            
            sqlite3_bind_text(stmt, 1, product.product_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, product.use_case_id.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, product.name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, product.description.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 5, structureStr.c_str(), -1, SQLITE_STATIC);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        
        return product;
    }
    
    std::vector<DataProductAttribute> designAttributes(const std::string& productId) {
        std::vector<DataProductAttribute> attributes;
        DataProduct product = getDataProduct(productId);
        if (product.product_id.empty()) return attributes;
        
        UseCaseAnalystAgent analyst;
        UseCase useCase = analyst.getUseCase(product.use_case_id);
        if (useCase.use_case_id.empty()) return attributes;
        
        for (const auto& req : useCase.requirements) {
            DataProductAttribute attr;
            attr.attribute_id = generateId("ATTR", req.first);
            attr.product_id = productId;
            attr.name = replaceSpaces(req.first);
            attr.description = req.second;
            
            // Determine data type
            attr.data_type = "string";
            if (containsAny(req.second, {"number", "count", "amount", "quantity"})) {
                attr.data_type = "numeric";
            } else if (containsAny(req.second, {"date", "time"})) {
                attr.data_type = "datetime";
            }
            
            attr.is_key = containsAny(req.first, {"key", "id", "identifier"});
            attr.sensitivity = containsAny(req.second, {"personal", "private"}) ? "high" : "medium";
            
            // Store attribute
            sqlite3_stmt* stmt;
            const char* query = "INSERT INTO data_product_attributes (attribute_id, product_id, "
                               "name, data_type, description, is_key, sensitivity) "
                               "VALUES (?, ?, ?, ?, ?, ?, ?)";
            
            if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, attr.attribute_id.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, attr.product_id.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, attr.name.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 4, attr.data_type.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 5, attr.description.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 6, attr.is_key ? 1 : 0);
                sqlite3_bind_text(stmt, 7, attr.sensitivity.c_str(), -1, SQLITE_STATIC);
                
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
            
            attributes.push_back(attr);
        }
        
        return attributes;
    }
    
    DataProduct getDataProduct(const std::string& productId) {
        DataProduct product;
        sqlite3_stmt* stmt;
        const char* query = "SELECT * FROM data_products WHERE product_id = ?";
        
        if (sqlite3_prepare_v2(dbHelper.getDB(), query, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, productId.c_str(), -1, SQLITE_STATIC);
            
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                product.product_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                product.use_case_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                product.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                product.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                
                std::string structureJson = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
                product.structure = parseJsonObject(structureJson);
                
                product.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
                product.certification_status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            }
        }
        sqlite3_finalize(stmt);
        return product;
    }
    
private:
    std::string mapToJson(const std::map<std::string, std::string>& map) {
        std::string result = "{";
        bool first = true;
        for (const auto& pair : map) {
            if (!first) result += ", ";
            result += "\"" + pair.first + "\": \"" + pair.second + "\"";
            first = false;
        }
        result += "}";
        return result;
    }
    
    std::map<std::string, std::string> parseJsonObject(const std::string& jsonStr) {
        std::map<std::string, std::string> result;
        size_t start = jsonStr.find('{');
        size_t end = jsonStr.find('}');
        
        if (start != std::string::npos && end != std::string::npos && end > start + 1) {
            std::string content = jsonStr.substr(start + 1, end - start - 1);
            size_t pos = 0;
            while ((pos = content.find('"')) != std::string::npos) {
                size_t keyEnd = content.find('"', pos + 1);
                if (keyEnd == std::string::npos) break;
                
                std::string key = content.substr(pos + 1, keyEnd - pos - 1);
                
                size_t valStart = content.find(':', keyEnd);
                if (valStart == std::string::npos) break;
                
                size_t valQuote = content.find('"', valStart + 1);
                if (valQuote == std::string::npos) break;
                
                size_t valEnd = content.find('"', valQuote + 1);
                if (valEnd == std::string::npos) break;
                
                std::string value = content.substr(valQuote + 1, valEnd - valQuote - 1);
                result[key] = value;
                
                content = content.substr(valEnd + 1);
            }
        }
        
        return result;
    }
    
    std::string replaceSpaces(const std::string& input) {
        std::string result = input;
        std::replace(result.begin(), result.end(), ' ', '_');
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    bool containsAny(const std::string& str, const std::vector<std::string>& terms) {
        for (const auto& term : terms) {
            if (str.find(term) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
};

// Data Product Orchestrator
class DataProductOrchestrator {
private:
    DatabaseHelper dbHelper;
    
public:
    void addSampleData() {
        // Add sample source systems
        const char* systems[] = {
            "INSERT OR IGNORE INTO source_systems (system_id, name, description, owner, data_domain, metadata) "
            "VALUES ('SYS001', 'CRM System', 'Customer relationship management system', 'Sales Team', 'Customer', "
            "'{\"refresh_frequency\": \"daily\"}')",
            
            "INSERT OR IGNORE INTO source_systems (system_id, name, description, owner, data_domain, metadata) "
            "VALUES ('SYS002', 'ERP System', 'Enterprise resource planning system', 'Finance Team', 'Financial', "
            "'{\"refresh_frequency\": \"hourly\"}')",
            
            "INSERT OR IGNORE INTO source_systems (system_id, name, description, owner, data_domain, metadata) "
            "VALUES ('SYS003', 'Web Analytics', 'Website visitor tracking system', 'Marketing Team', 'Digital', "
            "'{\"refresh_frequency\": \"real-time\"}')"
        };
        
        // Add sample source attributes
        const char* attributes[] = {
            "INSERT OR IGNORE INTO source_attributes (attribute_id, system_id, name, data_type, description, sample_values, sensitivity) "
            "VALUES ('ATTR001', 'SYS001', 'customer_id', 'string', 'Unique customer identifier', '[\"CUST001\", \"CUST002\"]', 'high')",
            
            "INSERT OR IGNORE INTO source_attributes (attribute_id, system_id, name, data_type, description, sample_values, sensitivity) "
            "VALUES ('ATTR002', 'SYS001', 'customer_name', 'string', 'Full name of customer', '[\"John Smith\", \"Jane Doe\"]', 'high')",
            
            "INSERT OR IGNORE INTO source_attributes (attribute_id, system_id, name, data_type, description, sample_values, sensitivity) "
            "VALUES ('ATTR003', 'SYS001', 'customer_segment', 'string', 'Marketing segment of customer', '[\"premium\", \"standard\"]', 'medium')",
            
            "INSERT OR IGNORE INTO source_attributes (attribute_id, system_id, name, data_type, description, sample_values, sensitivity) "
            "VALUES ('ATTR004', 'SYS002', 'transaction_amount', 'numeric', 'Value of financial transaction', '[\"100.50\", \"75.25\"]', 'medium')",
            
            "INSERT OR IGNORE INTO source_attributes (attribute_id, system_id, name, data_type, description, sample_values, sensitivity) "
            "VALUES ('ATTR005', 'SYS002', 'transaction_date', 'datetime', 'Date of transaction', '[\"2023-01-15\", \"2023-02-20\"]', 'medium')",
            
            "INSERT OR IGNORE INTO source_attributes (attribute_id, system_id, name, data_type, description, sample_values, sensitivity) "
            "VALUES ('ATTR006', 'SYS003', 'page_views', 'numeric', 'Number of page views per session', '[\"5\", \"10\", \"15\"]', 'low')"
        };
        
        // Execute all queries
        for (const auto& query : systems) {
            dbHelper.executeQuery(query);
        }
        
        for (const auto& query : attributes) {
            dbHelper.executeQuery(query);
        }
    }
    
    void runDemo() {
        std::cout << "Data Product Design System - Demonstration\n";
        std::cout << "----------------------------------------\n";
        
        // Create sample use case
        UseCase useCase;
        useCase.use_case_id = generateId("UC", "Customer Segmentation Analysis");
        useCase.title = "Customer Segmentation Analysis";
        useCase.description = "Analyze customer behavior to segment them for targeted marketing";
        useCase.business_domain = "Marketing";
        useCase.stakeholders = {"marketing_team", "sales_team"};
        useCase.requirements = {
            {"customer identifier", "Unique ID for each customer"},
            {"customer name", "Personal name of customer for reporting"},
            {"customer segment", "Current marketing segment of customer"},
            {"purchase history", "Aggregated purchase amount over last 12 months"},
            {"engagement score", "Calculated score based on website interactions"}
        };
        
        UseCaseAnalystCase analyst;
        std::string useCaseId = analyst.createUseCase(useCase);
        
        std::cout << "\nUse Case Created:\n";
        std::cout << "Title: " << useCase.title << "\n";
        std::cout << "Description: " << useCase.description << "\n";
        
        // Design data product
        DataProductDesignerAgent designer;
        DataProduct product = designer.designDataProduct(useCaseId);
        
        std::cout << "\nData Product Designed:\n";
        std::cout << "Name: " << product.name << "\n";
        std::cout << "Status: " << product.status << "\n";
        
        // Design attributes
        auto attributes = designer.designAttributes(product.product_id);
        
        std::cout << "\nAttributes Designed:\n";
        for (const auto& attr : attributes) {
            std::cout << "- " << attr.name << " (" << attr.data_type << "): " << attr.description << "\n";
        }
        
        std::cout << "\nDemo completed!\n";
    }
};

int main() {
    DataProductOrchestrator orchestrator;
    orchestrator.addSampleData();
    orchestrator.runDemo();
    return 0;
}