#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <memory>
#include <random>

// Enum for ticket priority
enum class TicketPriority {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

// Helper function to get priority name
std::string getPriorityName(TicketPriority priority) {
    switch (priority) {
        case TicketPriority::LOW: return "LOW";
        case TicketPriority::MEDIUM: return "MEDIUM";
        case TicketPriority::HIGH: return "HIGH";
        case TicketPriority::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

// Support Ticket class
class SupportTicket {
private:
    std::string ticket_id;
    std::string customer_id;
    std::string description;
    std::chrono::system_clock::time_point created_at;
    TicketPriority priority;
    std::string status;
    std::string assigned_team;
    std::string resolution;
    std::chrono::system_clock::time_point resolved_at;
    std::chrono::duration<double> resolution_time_estimate;
    std::vector<std::string> actions;
    std::string summary;

    TicketPriority determinePriority(const std::string& text) {
        std::string lowerText = text;
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
        
        if (lowerText.find("urgent") != std::string::npos ||
            lowerText.find("critical") != std::string::npos ||
            lowerText.find("down") != std::string::npos ||
            lowerText.find("outage") != std::string::npos) {
            return TicketPriority::CRITICAL;
        } else if (lowerText.find("error") != std::string::npos ||
                   lowerText.find("not working") != std::string::npos ||
                   lowerText.find("failed") != std::string::npos) {
            return TicketPriority::HIGH;
        } else if (lowerText.find("question") != std::string::npos ||
                   lowerText.find("information") != std::string::npos ||
                   lowerText.find("clarification") != std::string::npos) {
            return TicketPriority::LOW;
        }
        return TicketPriority::MEDIUM;
    }

public:
    SupportTicket(const std::string& ticket_id, const std::string& customer_id, 
                 const std::string& description, 
                 std::chrono::system_clock::time_point created_at = std::chrono::system_clock::now(),
                 TicketPriority priority = TicketPriority::MEDIUM)
        : ticket_id(ticket_id), customer_id(customer_id), description(description),
          created_at(created_at), priority(determinePriority(description)), 
          status("Open"), assigned_team(""), resolution(""), 
          resolution_time_estimate(0), summary("") {}

    // Getters
    std::string getTicketId() const { return ticket_id; }
    std::string getCustomerId() const { return customer_id; }
    std::string getDescription() const { return description; }
    std::chrono::system_clock::time_point getCreatedAt() const { return created_at; }
    TicketPriority getPriority() const { return priority; }
    std::string getStatus() const { return status; }
    std::string getAssignedTeam() const { return assigned_team; }
    std::string getResolution() const { return resolution; }
    std::chrono::system_clock::time_point getResolvedAt() const { return resolved_at; }
    std::chrono::duration<double> getResolutionTimeEstimate() const { return resolution_time_estimate; }
    std::vector<std::string> getActions() const { return actions; }
    std::string getSummary() const { return summary; }

    // Setters
    void setAssignedTeam(const std::string& team) { assigned_team = team; }
    void setResolution(const std::string& res) { resolution = res; }
    void setResolvedAt(std::chrono::system_clock::time_point time) { resolved_at = time; }
    void setStatus(const std::string& stat) { status = stat; }
    void setResolutionTimeEstimate(std::chrono::duration<double> estimate) { resolution_time_estimate = estimate; }
    void setActions(const std::vector<std::string>& acts) { actions = acts; }
    void setSummary(const std::string& summ) { summary = summ; }

    // Helper function to format time
    std::string formatTime(const std::chrono::system_clock::time_point& tp) const {
        std::time_t time = std::chrono::system_clock::to_time_t(tp);
        std::tm tm = *std::localtime(&time);
        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
        return std::string(buffer);
    }

    // Display ticket information
    void display() const {
        std::cout << "\nTicket ID: " << ticket_id;
        std::cout << "\nCustomer: " << customer_id;
        std::cout << "\nPriority: " << getPriorityName(priority);
        std::cout << "\nCreated At: " << formatTime(created_at);
        std::cout << "\nStatus: " << status;
        std::cout << "\nAssigned Team: " << assigned_team;
        
        if (!summary.empty()) {
            std::cout << "\nSummary: " << summary;
        }
        
        if (!actions.empty()) {
            std::cout << "\nActions: ";
            for (size_t i = 0; i < actions.size(); ++i) {
                if (i != 0) std::cout << ", ";
                std::cout << actions[i];
            }
        }
        
        if (!resolution.empty()) {
            std::cout << "\nResolution: " << resolution;
            std::cout << "\nResolved At: " << formatTime(resolved_at);
        }
        
        if (resolution_time_estimate.count() > 0) {
            std::cout << "\nEstimated Resolution Time: " 
                      << std::chrono::duration_cast<std::chrono::hours>(resolution_time_estimate).count() 
                      << " hours";
        }
        std::cout << std::endl;
    }
};

// Conversation Summarizer Agent (simplified)
class ConversationSummarizerAgent {
public:
    std::string generateSummary(const std::string& conversation) {
        // In a real implementation, this would use NLP
        // For demo purposes, we'll just return the first 3 sentences
        size_t first_period = conversation.find('.');
        size_t second_period = conversation.find('.', first_period + 1);
        size_t third_period = conversation.find('.', second_period + 1);
        
        if (third_period != std::string::npos) {
            return conversation.substr(0, third_period + 1);
        } else if (second_period != std::string::npos) {
            return conversation.substr(0, second_period + 1);
        } else if (first_period != std::string::npos) {
            return conversation.substr(0, first_period + 1);
        }
        return conversation;
    }
};

// Action Extractor Agent (simplified)
class ActionExtractorAgent {
public:
    std::vector<std::string> extractActions(const std::string& text) {
        // In a real implementation, this would use NLP
        // For demo, we'll look for key phrases
        std::vector<std::string> actions;
        std::string lowerText = text;
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
        
        if (lowerText.find("error") != std::string::npos) {
            actions.push_back("Investigate error");
        }
        if (lowerText.find("refund") != std::string::npos) {
            actions.push_back("Process refund");
        }
        if (lowerText.find("login") != std::string::npos || lowerText.find("password") != std::string::npos) {
            actions.push_back("Reset credentials");
        }
        if (lowerText.find("charge") != std::string::npos) {
            actions.push_back("Review charges");
        }
        
        if (actions.empty()) {
            actions.push_back("General follow-up required");
        }
        
        return actions;
    }
};

// Task Router Agent
class TaskRouterAgent {
private:
    std::map<std::string, std::vector<std::string>> teams = {
        {"Technical", {"error", "bug", "crash", "technical", "software", "hardware"}},
        {"Billing", {"payment", "invoice", "charge", "refund", "billing"}},
        {"Account", {"login", "password", "account", "access", "authentication"}},
        {"General", {"question", "information", "help", "support"}}
    };

public:
    std::string routeTicket(const std::string& description) {
        std::string lowerDesc = description;
        std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);
        
        for (const auto& team : teams) {
            for (const auto& keyword : team.second) {
                if (lowerDesc.find(keyword) != std::string::npos) {
                    return team.first;
                }
            }
        }
        
        return "General";
    }
};

// Resolution Time Estimator Agent
class ResolutionTimeEstimatorAgent {
private:
    std::map<TicketPriority, std::chrono::hours> baseTimes = {
        {TicketPriority::CRITICAL, std::chrono::hours(1)},
        {TicketPriority::HIGH, std::chrono::hours(4)},
        {TicketPriority::MEDIUM, std::chrono::hours(24)},
        {TicketPriority::LOW, std::chrono::hours(48)}
    };

public:
    std::chrono::duration<double> estimateResolutionTime(const SupportTicket& ticket) {
        // Simple estimation - in production would use ML model
        double complexityFactor = std::min(1.0 + ticket.getDescription().size() / 500.0, 2.0);
        return baseTimes[ticket.getPriority()] * complexityFactor;
    }
};

// Customer Support Environment
class CustomerSupportEnvironment {
private:
    ConversationSummarizerAgent summarizer;
    ActionExtractorAgent actionExtractor;
    TaskRouterAgent taskRouter;
    ResolutionTimeEstimatorAgent timeEstimator;
    std::vector<SupportTicket> openTickets;
    std::vector<SupportTicket> resolvedTickets;

public:
    SupportTicket createTicket(const std::string& customerId, const std::string& description) {
        std::string ticketId = "TKT-" + std::to_string(openTickets.size() + 1000);
        SupportTicket ticket(ticketId, customerId, description);
        
        // Generate summary
        ticket.setSummary(summarizer.generateSummary(description));
        
        // Extract actions
        ticket.setActions(actionExtractor.extractActions(description));
        
        // Route to appropriate team
        ticket.setAssignedTeam(taskRouter.routeTicket(description));
        
        // Estimate resolution time
        ticket.setResolutionTimeEstimate(timeEstimator.estimateResolutionTime(ticket));
        
        openTickets.push_back(ticket);
        return ticket;
    }
    
    bool resolveTicket(const std::string& ticketId, const std::string& resolution) {
        for (auto it = openTickets.begin(); it != openTickets.end(); ++it) {
            if (it->getTicketId() == ticketId) {
                it->setResolution(resolution);
                it->setStatus("Resolved");
                it->setResolvedAt(std::chrono::system_clock::now());
                resolvedTickets.push_back(*it);
                openTickets.erase(it);
                return true;
            }
        }
        return false;
    }
    
    void displayOpenTickets() const {
        std::cout << "\nOpen Tickets (" << openTickets.size() << "):\n";
        for (const auto& ticket : openTickets) {
            ticket.display();
        }
    }
    
    void displayResolvedTickets() const {
        std::cout << "\nResolved Tickets (" << resolvedTickets.size() << "):\n";
        for (const auto& ticket : resolvedTickets) {
            ticket.display();
        }
    }
};

// Example usage
int main() {
    CustomerSupportEnvironment env;

    // Create new tickets
    SupportTicket ticket1 = env.createTicket(
        "CUST-1001", 
        "I keep getting a 404 error when trying to access my dashboard. "
        "This started after your last update. It's critical for my business operations."
    );

    SupportTicket ticket2 = env.createTicket(
        "CUST-1002", 
        "I was charged twice for my subscription this month. "
        "Can you please refund the duplicate charge?"
    );

    // Display ticket information
    std::cout << "Ticket Processing Results:";
    ticket1.display();
    ticket2.display();

    // Resolve a ticket
    env.resolveTicket(ticket2.getTicketId(), "Processed refund for duplicate charge");
    std::cout << "\nTicket " << ticket2.getTicketId() << " has been resolved.\n";

    // Display all tickets
    env.displayOpenTickets();
    env.displayResolvedTickets();

    return 0;
}