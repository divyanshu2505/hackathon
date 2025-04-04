#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <ctime>
#include <sqlite3.h>
#include <algorithm>
#include <memory>
#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>

using namespace std;

// Helper function to get current timestamp
string currentTimestamp() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ltm);
    return string(buf);
}

// SQLite callback functions
int callback(void* data, int argc, char** argv, char** colName) {
    vector<map<string, string>>* result = static_cast<vector<map<string, string>>*>(data);
    map<string, string> row;
    for (int i = 0; i < argc; i++) {
        row[colName[i]] = argv[i] ? argv[i] : "NULL";
    }
    result->push_back(row);
    return 0;
}

// Database class to handle SQLite operations
class Database {
public:
    Database(const string& dbName) {
        if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
            cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        }
    }

    ~Database() {
        sqlite3_close(db);
    }

    vector<map<string, string>> executeQuery(const string& sql) {
        vector<map<string, string>> result;
        char* errMsg = 0;
        if (sqlite3_exec(db, sql.c_str(), callback, &result, &errMsg) != SQLITE_OK) {
            cerr << "SQL error: " << errMsg << endl;
            sqlite3_free(errMsg);
        }
        return result;
    }

    void execute(const string& sql) {
        char* errMsg = 0;
        if (sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
            cerr << "SQL error: " << errMsg << endl;
            sqlite3_free(errMsg);
        }
    }

private:
    sqlite3* db;
};

// Initialize database tables
void initializeDatabase(Database& db) {
    db.execute(R"(
        CREATE TABLE IF NOT EXISTS customers (
            customer_id TEXT PRIMARY KEY,
            name TEXT,
            age INTEGER,
            gender TEXT,
            location TEXT,
            segment TEXT,
            preferences TEXT,
            last_activity TIMESTAMP
        )
    )");

    db.execute(R"(
        CREATE TABLE IF NOT EXISTS products (
            product_id TEXT PRIMARY KEY,
            name TEXT,
            category TEXT,
            price REAL,
            description TEXT,
            tags TEXT,
            popularity_score REAL
        )
    )");

    db.execute(R"(
        CREATE TABLE IF NOT EXISTS interactions (
            interaction_id INTEGER PRIMARY KEY AUTOINCREMENT,
            customer_id TEXT,
            product_id TEXT,
            interaction_type TEXT,
            timestamp TIMESTAMP,
            duration INTEGER,
            FOREIGN KEY (customer_id) REFERENCES customers (customer_id),
            FOREIGN KEY (product_id) REFERENCES products (product_id)
        )
    )");

    db.execute(R"(
        CREATE TABLE IF NOT EXISTS purchases (
            purchase_id INTEGER PRIMARY KEY AUTOINCREMENT,
            customer_id TEXT,
            product_id TEXT,
            quantity INTEGER,
            amount REAL,
            timestamp TIMESTAMP,
            FOREIGN KEY (customer_id) REFERENCES customers (customer_id),
            FOREIGN KEY (product_id) REFERENCES products (product_id)
        )
    )");
}

// Interaction types
enum class InteractionType {
    VIEW, CART_ADD, WISHLIST, PURCHASE, SEARCH
};

string interactionTypeToString(InteractionType type) {
    switch (type) {
        case InteractionType::VIEW: return "view";
        case InteractionType::CART_ADD: return "cart_add";
        case InteractionType::WISHLIST: return "wishlist";
        case InteractionType::PURCHASE: return "purchase";
        case InteractionType::SEARCH: return "search";
        default: return "unknown";
    }
}

// Customer agent
class CustomerAgent {
public:
    CustomerAgent(Database& db, const string& customerId) 
        : db(db), customerId(customerId) {}

    map<string, string> getProfile() {
        string sql = "SELECT * FROM customers WHERE customer_id = '" + customerId + "'";
        auto result = db.executeQuery(sql);
        return result.empty() ? map<string, string>() : result[0];
    }

    void updateProfile(const map<string, string>& updates) {
        auto profile = getProfile();
        if (profile.empty()) {
            // Create new profile
            map<string, string> newProfile = {
                {"name", ""},
                {"age", "NULL"},
                {"gender", "NULL"},
                {"location", "NULL"},
                {"segment", "new"},
                {"preferences", "{}"},
                {"last_activity", currentTimestamp()},
                {"customer_id", customerId}
            };
            newProfile.insert(updates.begin(), updates.end());

            string columns, placeholders, values;
            for (const auto& [key, val] : newProfile) {
                if (!columns.empty()) columns += ", ";
                if (!placeholders.empty()) placeholders += ", ";
                columns += key;
                placeholders += "'" + val + "'";
            }

            string sql = "INSERT INTO customers (" + columns + ") VALUES (" + placeholders + ")";
            db.execute(sql);
        } else {
            // Update existing profile
            profile.insert(updates.begin(), updates.end());
            profile["last_activity"] = currentTimestamp();

            string setClause;
            for (const auto& [key, val] : profile) {
                if (key != "customer_id") {
                    if (!setClause.empty()) setClause += ", ";
                    setClause += key + " = '" + val + "'";
                }
            }

            string sql = "UPDATE customers SET " + setClause + " WHERE customer_id = '" + customerId + "'";
            db.execute(sql);
        }
    }

    void recordInteraction(const string& productId, InteractionType type, int duration = 0) {
        string sql = "INSERT INTO interactions (customer_id, product_id, interaction_type, timestamp, duration) VALUES ('" +
                     customerId + "', '" + productId + "', '" + interactionTypeToString(type) + "', '" + 
                     currentTimestamp() + "', " + to_string(duration) + ")";
        db.execute(sql);
    }

    void recordPurchase(const string& productId, int quantity, double amount) {
        string sql = "INSERT INTO purchases (customer_id, product_id, quantity, amount, timestamp) VALUES ('" +
                     customerId + "', '" + productId + "', " + to_string(quantity) + ", " + 
                     to_string(amount) + ", '" + currentTimestamp() + "')";
        db.execute(sql);
    }

private:
    Database& db;
    string customerId;
};

// Product agent
class ProductAgent {
public:
    ProductAgent(Database& db) : db(db) {
        prepareProductVectors();
    }

    vector<string> getSimilarProducts(const string& productId, int topN = 5) {
        if (productVectors.empty() || productIds.empty()) return {};

        auto it = find(productIds.begin(), productIds.end(), productId);
        if (it == productIds.end()) return {};

        int idx = distance(productIds.begin(), it);
        vector<double> similarities = calculateSimilarities(productVectors[idx]);

        vector<size_t> indices(similarities.size());
        iota(indices.begin(), indices.end(), 0);
        partial_sort(indices.begin(), indices.begin() + topN + 1, indices.end(),
            [&similarities](size_t a, size_t b) { return similarities[a] > similarities[b]; });

        vector<string> result;
        for (int i = 0; i < topN && i < indices.size(); i++) {
            if (indices[i] != idx) { // Exclude the product itself
                result.push_back(productIds[indices[i]]);
            }
        }
        return result;
    }

    map<string, string> getProductDetails(const string& productId) {
        string sql = "SELECT * FROM products WHERE product_id = '" + productId + "'";
        auto result = db.executeQuery(sql);
        return result.empty() ? map<string, string>() : result[0];
    }

    void addProduct(const map<string, string>& productData) {
        string sql = "INSERT INTO products (product_id, name, category, price, description, tags, popularity_score) VALUES ('" +
                     productData.at("product_id") + "', '" + productData.at("name") + "', '" + 
                     productData.at("category") + "', " + productData.at("price") + ", '" + 
                     productData.at("description") + "', '" + productData.at("tags") + "', " + 
                     productData.at("popularity_score") + ")";
        db.execute(sql);
        prepareProductVectors();
    }

private:
    Database& db;
    vector<string> productIds;
    vector<vector<double>> productVectors;

    void prepareProductVectors() {
        auto products = db.executeQuery("SELECT product_id, name, description, tags FROM products");
        productIds.clear();
        productVectors.clear();

        for (const auto& product : products) {
            productIds.push_back(product.at("product_id"));
            string text = product.at("name") + " " + product.at("description") + " " + product.at("tags");
            productVectors.push_back(createTextVector(text));
        }
    }

    vector<double> createTextVector(const string& text) {
        // Simplified TF-IDF vector creation (in real implementation use a proper library)
        vector<double> vector(128, 0.0); // Fixed size for simplicity
        hash<string> hasher;
        size_t hash = hasher(text);
        for (size_t i = 0; i < vector.size(); i++) {
            vector[i] = (hash % (i + 1)) / 100.0;
        }
        return vector;
    }

    vector<double> calculateSimilarities(const vector<double>& vec) {
        vector<double> similarities;
        for (const auto& otherVec : productVectors) {
            double dot = inner_product(vec.begin(), vec.end(), otherVec.begin(), 0.0);
            double norm1 = sqrt(inner_product(vec.begin(), vec.end(), vec.begin(), 0.0));
            double norm2 = sqrt(inner_product(otherVec.begin(), otherVec.end(), otherVec.begin(), 0.0));
            similarities.push_back(dot / (norm1 * norm2));
        }
        return similarities;
    }
};

// Segmentation agent
class SegmentationAgent {
public:
    SegmentationAgent(Database& db) : db(db) {}

    void updateCustomerSegments(int nClusters = 4) {
        auto data = db.executeQuery(R"(
            SELECT 
                c.customer_id,
                COUNT(DISTINCT i.interaction_id) as interaction_count,
                COUNT(DISTINCT p.purchase_id) as purchase_count,
                SUM(p.amount) as total_spent,
                COUNT(DISTINCT strftime('%Y-%m', p.timestamp)) as active_months
            FROM customers c
            LEFT JOIN interactions i ON c.customer_id = i.customer_id
            LEFT JOIN purchases p ON c.customer_id = p.customer_id
            GROUP BY c.customer_id
        )");

        if (data.empty()) return;

        // Prepare features for clustering
        vector<string> customerIds;
        vector<vector<double>> features;
        for (const auto& row : data) {
            customerIds.push_back(row.at("customer_id"));
            features.push_back({
                stod(row.at("interaction_count")),
                stod(row.at("purchase_count")),
                stod(row.at("total_spent")),
                stod(row.at("active_months"))
            });
        }

        // Normalize features
        normalizeFeatures(features);

        // Perform clustering (simplified)
        vector<int> segments = simpleKMeans(features, nClusters);

        // Update segments in database
        for (size_t i = 0; i < customerIds.size(); i++) {
            string sql = "UPDATE customers SET segment = 'segment_" + to_string(segments[i]) + 
                         "' WHERE customer_id = '" + customerIds[i] + "'";
            db.execute(sql);
        }
    }

private:
    Database& db;

    void normalizeFeatures(vector<vector<double>>& features) {
        if (features.empty()) return;

        size_t numFeatures = features[0].size();
        vector<double> means(numFeatures, 0.0);
        vector<double> stddevs(numFeatures, 0.0);

        // Calculate means
        for (const auto& vec : features) {
            for (size_t i = 0; i < numFeatures; i++) {
                means[i] += vec[i];
            }
        }
        for (auto& mean : means) mean /= features.size();

        // Calculate standard deviations
        for (const auto& vec : features) {
            for (size_t i = 0; i < numFeatures; i++) {
                stddevs[i] += pow(vec[i] - means[i], 2);
            }
        }
        for (auto& stddev : stddevs) stddev = sqrt(stddev / features.size());

        // Normalize
        for (auto& vec : features) {
            for (size_t i = 0; i < numFeatures; i++) {
                vec[i] = (stddevs[i] != 0) ? (vec[i] - means[i]) / stddevs[i] : 0;
            }
        }
    }

    vector<int> simpleKMeans(const vector<vector<double>>& features, int k) {
        if (features.empty() || k <= 0) return {};

        // Simplified k-means (in real implementation use a proper library)
        vector<int> clusters(features.size(), 0);
        vector<vector<double>> centroids(k, vector<double>(features[0].size(), 0.0));

        // Random initialization
        for (int i = 0; i < k; i++) {
            centroids[i] = features[rand() % features.size()];
        }

        // Assign clusters
        bool changed;
        do {
            changed = false;
            for (size_t i = 0; i < features.size(); i++) {
                int bestCluster = 0;
                double bestDistance = numeric_limits<double>::max();
                for (int j = 0; j < k; j++) {
                    double distance = 0.0;
                    for (size_t d = 0; d < features[i].size(); d++) {
                        distance += pow(features[i][d] - centroids[j][d], 2);
                    }
                    if (distance < bestDistance) {
                        bestDistance = distance;
                        bestCluster = j;
                    }
                }
                if (clusters[i] != bestCluster) {
                    clusters[i] = bestCluster;
                    changed = true;
                }
            }

            // Update centroids
            vector<vector<double>> newCentroids(k, vector<double>(features[0].size(), 0.0));
            vector<int> counts(k, 0);
            for (size_t i = 0; i < features.size(); i++) {
                for (size_t d = 0; d < features[i].size(); d++) {
                    newCentroids[clusters[i]][d] += features[i][d];
                }
                counts[clusters[i]]++;
            }
            for (int j = 0; j < k; j++) {
                if (counts[j] > 0) {
                    for (size_t d = 0; d < newCentroids[j].size(); d++) {
                        newCentroids[j][d] /= counts[j];
                    }
                }
            }
            centroids = newCentroids;
        } while (changed);

        return clusters;
    }
};

// Recommendation agent
class RecommendationAgent {
public:
    RecommendationAgent(Database& db) : db(db), productAgent(db) {}

    vector<string> getRecommendations(const string& customerId, int topN = 5) {
        // Get customer profile
        auto profile = db.executeQuery(
            "SELECT segment, preferences FROM customers WHERE customer_id = '" + customerId + "'");
        if (profile.empty()) return getFallbackRecommendations(topN);

        string segment = profile[0].at("segment");
        string preferences = profile[0].at("preferences");

        // Strategy 1: Personalized based on recent interactions
        auto recentProducts = getRecentInteractions(customerId);
        if (!recentProducts.empty()) {
            vector<string> similarProducts;
            for (const auto& productId : recentProducts) {
                auto similar = productAgent.getSimilarProducts(productId, topN);
                similarProducts.insert(similarProducts.end(), similar.begin(), similar.end());
            }
            if (!similarProducts.empty()) {
                // Remove duplicates and limit to topN
                sort(similarProducts.begin(), similarProducts.end());
                similarProducts.erase(unique(similarProducts.begin(), similarProducts.end()), similarProducts.end());
                if (similarProducts.size() > topN) {
                    similarProducts.resize(topN);
                }
                return similarProducts;
            }
        }

        // Strategy 2: Segment-based recommendations
        auto segmentProducts = getSegmentRecommendations(segment, topN);
        if (!segmentProducts.empty()) return segmentProducts;

        // Strategy 3: Fallback to popular items
        return getFallbackRecommendations(topN);
    }

private:
    Database& db;
    ProductAgent productAgent;

    vector<string> getRecentInteractions(const string& customerId, int limit = 3) {
        auto result = db.executeQuery(
            "SELECT product_id FROM interactions WHERE customer_id = '" + customerId + 
            "' ORDER BY timestamp DESC LIMIT " + to_string(limit));
        vector<string> products;
        for (const auto& row : result) {
            products.push_back(row.at("product_id"));
        }
        return products;
    }

    vector<string> getSegmentRecommendations(const string& segment, int topN) {
        auto result = db.executeQuery(
            "SELECT p.product_id FROM products p "
            "JOIN purchases pu ON p.product_id = pu.product_id "
            "JOIN customers c ON pu.customer_id = c.customer_id "
            "WHERE c.segment = '" + segment + "' "
            "GROUP BY p.product_id ORDER BY COUNT(pu.purchase_id) DESC "
            "LIMIT " + to_string(topN));
        vector<string> products;
        for (const auto& row : result) {
            products.push_back(row.at("product_id"));
        }
        return products;
    }

    vector<string> getFallbackRecommendations(int topN) {
        auto result = db.executeQuery(
            "SELECT product_id FROM products ORDER BY popularity_score DESC LIMIT " + to_string(topN));
        vector<string> products;
        for (const auto& row : result) {
            products.push_back(row.at("product_id"));
        }
        return products;
    }
};

// E-commerce environment
class ECommerceEnvironment {
public:
    ECommerceEnvironment() : db("ecommerce_recommendations.db") {
        initializeDatabase(db);
    }

    void addSampleData() {
        // Add sample products
        ProductAgent productAgent(db);
        vector<map<string, string>> products = {
            {
                {"product_id", "P1001"},
                {"name", "Wireless Headphones"},
                {"category", "Electronics"},
                {"price", "99.99"},
                {"description", "Premium wireless headphones with noise cancellation"},
                {"tags", "[\"audio\", \"wireless\", \"bluetooth\"]"},
                {"popularity_score", "8.5"}
            },
            {
                {"product_id", "P1002"},
                {"name", "Smartphone"},
                {"category", "Electronics"},
                {"price", "699.99"},
                {"description", "Latest smartphone with high-resolution camera"},
                {"tags", "[\"mobile\", \"android\", \"camera\"]"},
                {"popularity_score", "9.2"}
            },
            {
                {"product_id", "P1003"},
                {"name", "Running Shoes"},
                {"category", "Sports"},
                {"price", "79.99"},
                {"description", "Lightweight running shoes for marathon training"},
                {"tags", "[\"fitness\", \"running\", \"shoes\"]"},
                {"popularity_score", "7.8"}
            }
        };

        for (const auto& product : products) {
            productAgent.addProduct(product);
        }

        // Add sample customers and interactions
        CustomerAgent customer1(db, "CUST001");
        customer1.updateProfile({
            {"name", "John Doe"},
            {"age", "32"},
            {"gender", "male"},
            {"location", "New York"}
        });
        customer1.recordInteraction("P1001", InteractionType::VIEW, 120);
        customer1.recordInteraction("P1001", InteractionType::CART_ADD);
        customer1.recordPurchase("P1001", 1, 99.99);

        CustomerAgent customer2(db, "CUST002");
        customer2.updateProfile({
            {"name", "Jane Smith"},
            {"age", "28"},
            {"gender", "female"},
            {"location", "Los Angeles"}
        });
        customer2.recordInteraction("P1002", InteractionType::VIEW, 180);
        customer2.recordInteraction("P1003", InteractionType::WISHLIST);
    }

    void runDemo() {
        // Update customer segments
        SegmentationAgent segmentationAgent(db);
        segmentationAgent.updateCustomerSegments();

        // Get recommendations for customers
        RecommendationAgent recommendationAgent(db);
        auto customer1Recs = recommendationAgent.getRecommendations("CUST001");
        auto customer2Recs = recommendationAgent.getRecommendations("CUST002");

        // Display results
        ProductAgent productAgent(db);
        cout << "\nRecommendation System Demo:" << endl;
        
        cout << "\nRecommendations for John Doe (CUST001):" << endl;
        for (const auto& productId : customer1Recs) {
            auto product = productAgent.getProductDetails(productId);
            cout << "- " << product["name"] << " ($" << product["price"] << ")" << endl;
        }
        
        cout << "\nRecommendations for Jane Smith (CUST002):" << endl;
        for (const auto& productId : customer2Recs) {
            auto product = productAgent.getProductDetails(productId);
            cout << "- " << product["name"] << " ($" << product["price"] << ")" << endl;
        }
    }

private:
    Database db;
};

int main() {
    ECommerceEnvironment env;
    env.addSampleData();
    env.runDemo();
    return 0;
}