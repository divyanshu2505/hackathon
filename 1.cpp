//Optimizing Retail Inventory with Multi Agents


#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <ctime>
#include <random>
#include <algorithm>
#include <iomanip>
#include <memory>

using namespace std;

// Helper function to get current date
string currentDate() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buf[11];
    strftime(buf, sizeof(buf), "%Y-%m-%d", ltm);
    return string(buf);
}

// Helper function to add days to a date
string addDaysToDate(const string& dateStr, int days) {
    tm tm = {};
    istringstream iss(dateStr);
    iss >> get_time(&tm, "%Y-%m-%d");
    time_t time = mktime(&tm) + days * 24 * 60 * 60;
    tm = *localtime(&time);
    ostringstream oss;
    oss << put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

// Helper function to get day of week (0-6, Sunday=0)
int getDayOfWeek(const string& dateStr) {
    tm tm = {};
    istringstream iss(dateStr);
    iss >> get_time(&tm, "%Y-%m-%d");
    mktime(&tm);
    return tm.tm_wday;
}

// Helper function to get month (1-12)
int getMonth(const string& dateStr) {
    tm tm = {};
    istringstream iss(dateStr);
    iss >> get_time(&tm, "%Y-%m-%d");
    mktime(&tm);
    return tm.tm_mon + 1;
}

// Random number generator
int randomInt(int min, int max) {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

double randomDouble(double min, double max) {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_real_distribution<> dis(min, max);
    return dis(gen);
}

// Simplified Random Forest Regression model (placeholder)
class RandomForestRegressor {
public:
    void fit(const vector<vector<double>>& X, const vector<int>& y) {
        // In a real implementation, this would train a model
        // For this example, we'll just store the data
        this->X = X;
        this->y = y;
    }

    int predict(const vector<double>& features) const {
        // Simplified prediction - average of similar historical records
        double sum = 0;
        int count = 0;
        
        for (size_t i = 0; i < X.size(); ++i) {
            double similarity = 0;
            for (size_t j = 0; j < features.size(); ++j) {
                similarity += abs(X[i][j] - features[j]);
            }
            if (similarity < 2.0) { // Arbitrary threshold
                sum += y[i];
                count++;
            }
        }
        
        if (count > 0) {
            return max(0, static_cast<int>(sum / count));
        }
        return 10; // Default prediction
    }

private:
    vector<vector<double>> X;
    vector<int> y;
};

class DemandForecastingAgent {
public:
    DemandForecastingAgent() = default;

    void trainModel(const vector<map<string, double>>& salesData) {
        vector<vector<double>> X;
        vector<int> y;
        
        for (const auto& record : salesData) {
            vector<double> features = {
                record.at("day_of_week"),
                record.at("month"),
                record.at("is_weekend"),
                record.at("price"),
                record.at("promotion")
            };
            X.push_back(features);
            y.push_back(static_cast<int>(record.at("quantity")));
        }
        
        model.fit(X, y);
    }

    int predictDemand(const map<string, double>& productInfo, const string& futureDate) const {
        vector<double> features = {
            static_cast<double>(getDayOfWeek(futureDate)),
            static_cast<double>(getMonth(futureDate)),
            (getDayOfWeek(futureDate) == 0 || getDayOfWeek(futureDate) == 6) ? 1.0 : 0.0,
            productInfo.at("price"),
            productInfo.at("promotion")
        };
        return model.predict(features);
    }

private:
    RandomForestRegressor model;
};

class InventoryMonitoringAgent {
public:
    void updateInventory(const string& productId, int quantity) {
        inventory[productId] += quantity;
    }

    void setThresholds(const string& productId, int minThreshold, int maxThreshold) {
        thresholds[productId] = make_pair(minThreshold, maxThreshold);
    }

    pair<string, int> checkInventory(const string& productId) const {
        int current = inventory.at(productId);
        auto [min_thresh, max_thresh] = thresholds.at(productId);
        
        if (current < min_thresh) {
            return {"low", max_thresh - current};
        } else if (current > max_thresh) {
            return {"high", current - max_thresh};
        }
        return {"ok", 0};
    }

    map<string, int> inventory;
    map<string, pair<int, int>> thresholds;
};

class PricingOptimizationAgent {
public:
    void setBasePrice(const string& productId, double basePrice) {
        priceStrategies[productId]["base_price"] = basePrice;
    }

    double calculateOptimalPrice(const string& productId, int demandForecast, int currentInventory, int daysInStock) const {
        double basePrice = priceStrategies.at(productId).at("base_price");
        
        if (daysInStock > 30) { // Slow-moving
            return basePrice * 0.8; // 20% discount
        } else if (daysInStock > 60) { // Very slow-moving
            return basePrice * 0.7; // 30% discount
        } else if (currentInventory < demandForecast * 0.5) { // Potential stockout
            return basePrice * 1.1; // 10% increase
        }
        return basePrice;
    }

private:
    map<string, map<string, double>> priceStrategies;
};

class SupplierCoordinationAgent {
public:
    void registerSupplier(const string& supplierId, int leadTime, int minOrderQuantity) {
        suppliers[supplierId] = {leadTime, minOrderQuantity};
    }

    pair<bool, string> placeOrder(const string& productId, int quantity) {
        if (suppliers.empty()) {
            return {false, "No suppliers registered"};
        }
        
        // For simplicity, use the first supplier
        auto& [leadTime, minOrderQty] = suppliers.begin()->second;
        return {true, "Order placed with " + suppliers.begin()->first + 
                      ". Expected delivery in " + to_string(leadTime) + " days."};
    }

private:
    map<string, pair<int, int>> suppliers; // supplierId -> (leadTime, minOrderQuantity)
};

class RetailEnvironment {
public:
    RetailEnvironment() = default;

    void initializeSystem(const vector<map<string, string>>& products) {
        // Generate synthetic sales data for demonstration
        auto salesData = generateSalesData(products);
        demandAgent.trainModel(salesData);
        
        // Set initial inventory levels and thresholds
        for (const auto& product : products) {
            inventoryAgent.updateInventory(product.at("id"), stoi(product.at("initial_stock")));
            inventoryAgent.setThresholds(product.at("id"), 
                                      stoi(product.at("min_threshold")), 
                                      stoi(product.at("max_threshold")));
            pricingAgent.setBasePrice(product.at("id"), stod(product.at("base_price")));
        }
        
        // Register a sample supplier
        supplierAgent.registerSupplier("SUP-001", 3, 10);
    }

    vector<map<string, double>> generateSalesData(const vector<map<string, string>>& products, int days = 90) {
        vector<map<string, double>> data;
        string startDate = currentDate();
        
        for (int day = 0; day < days; ++day) {
            string date = addDaysToDate(startDate, -days + day);
            for (const auto& product : products) {
                // Simulate sales with some seasonality
                int baseSales = stoi(product.at("base_demand"));
                double dayFactor = (getDayOfWeek(date) == 0 || getDayOfWeek(date) == 6) ? 1.5 : 1.0;
                double monthFactor = (getMonth(date) == 11 || getMonth(date) == 12) ? 1.2 : 1.0;
                
                int sales = static_cast<int>(baseSales * dayFactor * monthFactor * randomDouble(0.8, 1.2));
                
                map<string, double> record = {
                    {"date", 0}, // Placeholder, we'll use the date string separately
                    {"product_id", 0}, // Placeholder
                    {"quantity", static_cast<double>(sales)},
                    {"price", stod(product.at("base_price"))},
                    {"promotion", static_cast<double>(randomInt(0, 1))},
                    {"day_of_week", static_cast<double>(getDayOfWeek(date))},
                    {"month", static_cast<double>(getMonth(date))},
                    {"is_weekend", (getDayOfWeek(date) == 0 || getDayOfWeek(date) == 6) ? 1.0 : 0.0}
                };
                data.push_back(record);
            }
        }
        
        return data;
    }

    vector<map<string, string>> runSimulation(int days = 30) {
        vector<map<string, string>> results;
        string currentDateStr = currentDate();
        
        for (int day = 0; day < days; ++day) {
            string simDate = addDaysToDate(currentDateStr, day);
            map<string, string> dayResults;
            dayResults["date"] = simDate;
            
            // Check inventory for all products
            for (const auto& [productId, _] : inventoryAgent.inventory) {
                auto [status, quantity] = inventoryAgent.checkInventory(productId);
                
                if (status == "low") {
                    // Get demand forecast for next week
                    int forecast = demandAgent.predictDemand(
                        {{"price", pricingAgent.calculateOptimalPrice(productId, 0, 0, 0)},
                         {"promotion", 0}},
                        addDaysToDate(simDate, 7)
                    );
                    
                    // Calculate order quantity
                    int orderQty = max(forecast * 1.2 - inventoryAgent.inventory[productId], 3); // Simplified min order
                    
                    // Place order
                    auto [success, orderStatus] = supplierAgent.placeOrder(productId, orderQty);
                    
                    // Update inventory (simulating delivery after lead time)
                    if (success && day > 3) { // Simplified lead time
                        inventoryAgent.updateInventory(productId, orderQty);
                    }
                } else if (status == "high") {
                    // Adjust price to clear excess inventory
                    int daysInStock = randomInt(15, 60);
                    double newPrice = pricingAgent.calculateOptimalPrice(
                        productId, 
                        10, // Simplified forecast
                        inventoryAgent.inventory[productId],
                        daysInStock
                    );
                    
                    // Update price strategy
                    pricingAgent.setBasePrice(productId, newPrice);
                }
                
                // Record results
                dayResults[productId + "_inventory"] = to_string(inventoryAgent.inventory[productId]);
                dayResults[productId + "_status"] = status;
                dayResults[productId + "_price"] = to_string(pricingAgent.calculateOptimalPrice(productId, 0, 0, 0));
            }
            
            results.push_back(dayResults);
        }
        
        return results;
    }

private:
    DemandForecastingAgent demandAgent;
    InventoryMonitoringAgent inventoryAgent;
    PricingOptimizationAgent pricingAgent;
    SupplierCoordinationAgent supplierAgent;
};

int main() {
    // Sample product data
    vector<map<string, string>> products = {
        {
            {"id", "P001"},
            {"name", "T-Shirt"},
            {"base_price", "20"},
            {"base_demand", "15"},
            {"initial_stock", "50"},
            {"min_threshold", "20"},
            {"max_threshold", "100"}
        },
        {
            {"id", "P002"},
            {"name", "Jeans"},
            {"base_price", "50"},
            {"base_demand", "8"},
            {"initial_stock", "30"},
            {"min_threshold", "10"},
            {"max_threshold", "60"}
        }
    };
    
    // Initialize the retail environment
    RetailEnvironment env;
    env.initializeSystem(products);
    
    // Run a 30-day simulation
    auto simulationResults = env.runSimulation(30);
    
    // Display results
    cout << "\nSimulation Results:" << endl;
    for (const auto& day : simulationResults) {
        cout << "\nDate: " << day.at("date") << endl;
        for (const auto& [key, value] : day) {
            if (key != "date") {
                cout << key << ": " << value << endl;
            }
        }
    }
    
    return 0;
}
