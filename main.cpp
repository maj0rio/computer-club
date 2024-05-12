#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>


class Event {
public:
    virtual ~Event() {}
    virtual std::string type() const = 0;

    std::string clientName;
    int time;
};

class ClientArrivedEvent : public Event {
public:
    std::string type() const override { return "1"; }
};

class ClientSatDownEvent : public Event {
public:
    std::string type() const override { return "2"; }
    int tableNumber;
};

class ClientIsWaitingEvent : public Event {
public:
    std::string type() const override { return "3"; }
};

class ClientLeftEvent : public Event {
public:
    std::string type() const override { return "4"; }
};

class ComputerClub {
public:

    ComputerClub(int num_tables, std::pair<int, int> schedule, int hourly_rate);

    /**
     * @brief handler for all valid events
     * @param event 
     * @return true if event is correct
     * @return false if event caused error
     */
    void processEvent(std::shared_ptr<Event> event, std::string event_string);

    /**
     * @brief handle for coming event 
     * check working time and check client in the club
     * @param event 
     * @return 0 all is good
     * @return 1 NotOpenYet
     * @return 2 YouShallNotPass
     */
    int ClientComing(std::shared_ptr<Event> event);

    /**
     * @brief handle for sit down event
     * checks that table availability 
     * checks that client in club
     * put current_time in in table_id, match client to new table  
     * @param event 
     * @return 0 all is good
     * @return 1 PlaceIsBusy
     * @return 2 ClientUnknown 
     */
    int ClientSittingDown(std::shared_ptr<ClientSatDownEvent> event);
    
    /**
     * @brief handle for waiting clients
     * check availible tables, check lenght of waiting queue
     * 
     * add client to waiting queue
     * @param event 
     * @return 0 (OK)
     * @return 1 ICanWaitNoLonger
     * @return 2 Client goes away
     */
    int ClientWaiting(std::shared_ptr<Event> event);
    
    /**
     * @brief handle for leaving client
     * at first, check that client in club
     * then clear the table and seat the first person in queue
     * @param event 
     * @return {0, table_id} OK and nessesary FirstAtWaitingQueue
     * @return {1, -1} ClientUnknown
     * @return {2, -1} OK but without FirstAtWaitingQueue
     */
    std::pair<int, int> ClientLeaving(std::shared_ptr<Event> event);
    
    /**
     * @brief handle calls if day is over or queue is full
     * 
     * @param status_code 1 if day is over, 2 if queue is full
     * @param client_name only when status_code = 2
     */
    void ForcedLeaving(int status_code, std::string client_name = "");

    /**
     * @brief When a table becomes free, sit down first queue
     * 
     * @param table_id 
     */
    void FirstAtWaitingQueue(int time, int table_id);

    /**
     * @brief counting hours spent at the table
     * @param start_time 
     * @param end_time 
     * @return int 
     */
    int CalculateHours(int start_time, int end_time);

    /**
     * @brief working time validation
     * 
     * @param minute 
     * @return true time is valid
     * @return false time is invalid
     */
    bool CheckWorkingTime(int minute);

    /**
     * @brief convert time from count minutes to view ТТ:MM 
     * 
     * @param time count minutes
     * @return std::string 
     */
    std::string IntTimeToString(int time);


public:
    int num_tables_;
    int open_time_;
    int close_time_;
    std::vector<int> revenue_table; // table revenue. public because for counting from outside class
    std::vector<int> working_table_time; // counts working time for all tablse

private:
    int hourly_rate_;
    std::map<std::string, int> clients; // match client name to table number or (client name, -1) if client isn't playing
    std::vector<int> tables; // availability table numbers (-1 when availible)
    std::queue<std::string> waitingQueue; // client names in waiting queue
};



ComputerClub::ComputerClub(int num_tables, std::pair<int, int> schedule, int hourly_rate): num_tables_(num_tables), open_time_(schedule.first), 
    close_time_(schedule.second), hourly_rate_(hourly_rate) {
    tables.resize(num_tables_, -1);
    revenue_table.resize(num_tables, 0);
    working_table_time.resize(num_tables, 0);
}

void ComputerClub::processEvent(std::shared_ptr<Event> event, std::string event_string) {
    if (event->type() == "1") {
        int status_code = ClientComing(event);
        std::cout << event_string << "\n";

        if (status_code == 1)
            std::cout << event_string.substr(0, event_string.find(' ')) << " 13 NotOpenYet\n";
        if (status_code == 2)
            std::cout << event_string.substr(0, event_string.find(' ')) << " 13 YouShallNotPass\n";   
            
    } else if (event->type() == "2") {
        std::shared_ptr<ClientSatDownEvent> derived_event = std::dynamic_pointer_cast<ClientSatDownEvent>(event);
        int status_code = ClientSittingDown(derived_event);
        std::cout << event_string << "\n";

        if (status_code == 1)
            std::cout << event_string.substr(0, event_string.find(' ')) << " 13 PlaceIsBusy\n";
        if (status_code == 2)
            std::cout << event_string.substr(0, event_string.find(' ')) << " 13 ClientUnknown\n";

    } else if (event->type() == "3") {
        int status_code = ClientWaiting(event);
        std::cout << event_string << "\n";

        if (status_code == 1)
            std::cout << event_string.substr(0, event_string.find(' ')) << " 13 ICanWaitNoLonger!\n";

        if (status_code == 2)
            ForcedLeaving(2, event->clientName);
    
    } else if (event->type() == "4") {
        auto [status_code, table_id] = ClientLeaving(event);
        if (status_code == 0) {
            std::cout << event_string << "\n";
            FirstAtWaitingQueue(event->time, table_id);
        }
        if (status_code == 2) {
            std::cout << event_string << "\n";
        }
        if (status_code == 1)
            std::cout << event_string.substr(0, event_string.find(' ')) << " 13 ClientUnknown\n";
    }
}

int ComputerClub::ClientComing(std::shared_ptr<Event> event) {
    if (!CheckWorkingTime(event->time))
            return 1;
        
        if (clients.find(event->clientName) != clients.end())
            return 2;
        
        clients[event->clientName] = -1; // -1 means client just arrived
        return 0;
}

int ComputerClub::ClientSittingDown(std::shared_ptr<ClientSatDownEvent> event) {
    int table_num = event->tableNumber - 1;
    if (clients.find(event->clientName) == clients.end())
        return 2;
    if (tables[table_num] != -1 || clients[event->clientName] == table_num)
        return 1;
    
    if (clients[event->clientName] != -1) {
        // client has already sat at the table and he wants to change
        int old_table = clients[event->clientName];
        int hours = CalculateHours(tables[old_table], event->time);
        revenue_table[old_table] += hourly_rate_ * hours;
        working_table_time[old_table] += event->time - tables[old_table];
        tables[old_table] = -1;
    }
    clients[event->clientName] = table_num;
    tables[table_num] = event->time;
    return 0;
}

int ComputerClub::ClientWaiting(std::shared_ptr<Event> event) {
    int cnt_empty_tables = std::count_if(tables.begin(), tables.end(), [](int a) { return a == -1; });
    if (cnt_empty_tables)
        return 1;
    
    if (waitingQueue.size() > num_tables_)
        return 2;
    
    waitingQueue.push(event->clientName);
    return 0;
}

std::pair<int, int> ComputerClub::ClientLeaving(std::shared_ptr<Event> event) {
    if (clients.find(event->clientName) == clients.end())
        return {1, -1};
    
    int table_num = clients[event->clientName];
    if (table_num == -1) {
    // client did not sit at a table, so he just left
        clients.erase(event->clientName);
        return {2, -1};
    }
    
    int hours = CalculateHours(tables[table_num], event->time);
    revenue_table[table_num] += hours * hourly_rate_;
    working_table_time[table_num] += event->time - tables[table_num]; 
    tables[table_num] = -1;
    clients.erase(event->clientName);
    if (waitingQueue.size() != 0 and table_num != -1)
        return {0, table_num};
    
    return {2, -1};
}

void ComputerClub::ForcedLeaving(int status_code, std::string client_name) {
    std::string time_string = IntTimeToString(close_time_);  
    if (status_code == 1) {
        std::map<std::string, int>::iterator it = clients.begin();
        while (it != clients.end()) {
            std::cout << time_string + " 11 " + it->first << "\n";
            if (it ->second != -1) {
                int hours = CalculateHours(tables[it->second], close_time_);
                revenue_table[it->second] += hours * hourly_rate_;
                working_table_time[it->second] += close_time_ - tables[it->second];
                tables[it->second] = -1;
            }
            it = clients.erase(it);
        }
    } else {
        std::cout << time_string + " 11 " + client_name << "\n";
        clients.erase(client_name);
    }
}

void ComputerClub::FirstAtWaitingQueue(int time, int table_id) {
    std::string time_string = IntTimeToString(time);
    std::string first_at_queue = waitingQueue.front();
    waitingQueue.pop();
    tables[table_id] = time;
    clients[first_at_queue] = table_id;

    std::cout << time_string << " 12 " << first_at_queue << " " << table_id + 1 << "\n";
}

int ComputerClub::CalculateHours(int start_time, int end_time) { 
    int hours = static_cast<int>(ceil((end_time - start_time) / 60.0));
    return hours;
}

bool ComputerClub::CheckWorkingTime(int minute) {
    if (minute >= open_time_ && minute <= close_time_)
        return true;
    return false;
}

std::string ComputerClub::IntTimeToString(int time) {
    int hour = time / 60;
    int minute = time % 60;
    std::string time_string = (hour < 10) ? "0" + std::to_string(hour) + ":" : std::to_string(hour) + ":";
    time_string += (minute < 10) ? "0" + std::to_string(minute) : std::to_string(minute);
    return time_string;
}

/**
 * @brief time validation
 * 
 * @param open_time_hour 
 * @param open_time_minute 
 * @param close_time_hour 
 * @param close_time_minute 
 * @return true - if time is valid 
 * @return false  - if time is incorrect
 */
bool CheckInputTime(int hour, int minute) {
    if (hour >= 24 || hour < 0 || minute >= 60 || minute < 0)
        return false;
    return true;
}

/**
 * @brief checking valid of clients name
 * 
 * @param name 
 * @return true name is valid
 * @return false name is invalid
 */
bool CheckClientName(const std::string& name) {
    for (size_t i = 0; i < name.size(); ++i) {
        if (!(name[i] >= 'a' && name[i] <= 'z') && !(name[i] >= '0' && name[i] <= '9') && name[i] != '_' && name[i] != '-')
            return false;    
    }
    return true;
}

/**
 * @brief check that time has view ТТ:MM and beetwen 00:00 - 23:59
 * @param time_string 
 * @return true 
 * @return false 
 */
bool ValidateTimeString(std::string hours, std::string minutes) {
    if (hours.size() != 2 || minutes.size() != 2)
        return false;
    if (!CheckInputTime(std::stoi(hours), std::stoi(minutes)))
        return false;
    return true;
}

/**
 * @brief just parse schedule string on open_time and close_time
 * 
 * @param input_line 
 * @return std::pair<int, int> 
 */
std::pair<int, int> ReadInputTime(const std::string& input_line) {
    char open_time_a, open_time_b, open_time_c, open_time_d;
    char close_time_a, close_time_b, close_time_c, close_time_d;
    char separatorColon, separatorSpace;
    std::istringstream is(input_line);
    if (!(is >> std::noskipws >> open_time_a >> open_time_b >> separatorColon >> open_time_c >> open_time_d) || separatorColon != ':') {
        throw std::runtime_error("Error at line 2: invalid time format");
    }
    if (is.get() != ' ') {
        throw std::runtime_error("Error at line 2: invalid time format");
    }
    if (!(is >> close_time_a >> close_time_b >> separatorColon >> close_time_c >> close_time_d) || separatorColon != ':') {
        throw std::runtime_error("Error at line 2: invalid time format");
    }
    std::string open_time_hour_str = std::string(1, open_time_a) + std::string(1, open_time_b);
    std::string open_time_minute_str = std::string(1, open_time_c) + std::string(1, open_time_d);
    std::string close_time_hour_str = std::string(1, close_time_a) + std::string(1, close_time_b);
    std::string close_time_minute_str = std::string(1, close_time_c) + std::string(1, close_time_d);
    if (!ValidateTimeString(open_time_hour_str, open_time_minute_str) || !ValidateTimeString(close_time_hour_str, close_time_minute_str)) {
        throw std::runtime_error("Error at line 2: invalid time format");
    }
    int open_time_hour_int = std::stoi(open_time_hour_str);
    int open_time_minute_int = std::stoi(open_time_minute_str);
    int close_time_hour_int = std::stoi(close_time_hour_str);
    int close_time_minute_int = std::stoi(close_time_minute_str);

    if (open_time_hour_int * 60 + open_time_minute_int > close_time_hour_int * 60 + close_time_minute_int) {
        throw std::runtime_error("Error at line 2: invalid time format");
    }

    std::pair<int, int> schedule = std::make_pair<int, int>(open_time_hour_int * 60 + open_time_minute_int, close_time_hour_int * 60 + close_time_minute_int);
    return schedule;
}

/**
 * @brief check that event_string is valid and return false if it is invalid
 * 
 * @param club 
 * @param event_string 
 * @return true 
 * @return false 
 */
bool CheckEventString(const ComputerClub& club, std::string& event_string) {
    char open_time_a, open_time_b, open_time_c, open_time_d;
    char separatorColon;
    std::istringstream is(event_string);
    if (!(is >> std::noskipws >> open_time_a >> open_time_b >> separatorColon >> open_time_c >> open_time_d) || separatorColon != ':') {
        return false;
    }
    std::string open_time_hour_str = std::string(1, open_time_a) + std::string(1, open_time_b);
    std::string open_time_minute_str = std::string(1, open_time_c) + std::string(1, open_time_d);
    if (!ValidateTimeString(open_time_hour_str, open_time_minute_str)) {
        return false;
    }
    char space_separate;
    int id;
    if (!(is >> space_separate >> id) || space_separate != ' ')
        return false;

    std::string client_name;
    if (!(is >> space_separate >> client_name) || space_separate != ' ')
        return false;
    
    if (!CheckClientName(client_name))
        return false;

    if (id < 1 || id > 4)
        return false;
    
    if (id != 2 && !is.eof())
        return false;

    int table_number;
    if (id == 2 && (!(is >> space_separate >> table_number) || space_separate != ' ' || !(table_number >= 1 && table_number <= club.num_tables_))) {
        return false;
    }
    
    return true;
}

/**
 * @brief make Event from valid event_string
 * just define type of event and put corresponding fields
 * @param event_string 
 * @return std::shared_ptr<Event> pointer on corresponding Event class
 */
std::shared_ptr<Event> MakeEvent(const std::string& event_string) {
    std::stringstream ss(event_string);

    int hour, minute;
    char sepatate_colon;
    ss >> hour >> sepatate_colon >> minute;

    int type;
    ss >> type;
    switch (type) {
        case 1: {
            // ClientArrivedEvent
            std::string clientName;
            ss >> clientName;

            auto event = std::make_shared<ClientArrivedEvent>();
            event->clientName = clientName;
            event->time = hour * 60 + minute;
            return event;
        }
        case 2: {
            // ClientSatDownEvent
            std::string clientName;
            int tableNumber;
            ss >> clientName >> tableNumber;

            auto event = std::make_shared<ClientSatDownEvent>();
            event->clientName = clientName;
            event->time = hour * 60 + minute;
            event->tableNumber = tableNumber;
            return event;
        }
        case 3: {
            // ClientIsWaitingEvent
            std::string clientName;
            ss >> clientName;

            auto event = std::make_shared<ClientIsWaitingEvent>();
            event->clientName = clientName;
            event->time = hour * 60 + minute;
            return event;
        }
        case 4: {
            // ClientLeftEvent
            std::string clientName;
            ss >> clientName;

            auto event = std::make_shared<ClientLeftEvent>();
            event->clientName = clientName;
            event->time = hour * 60 + minute;
            return event;
        }
        default:
            throw std::runtime_error("Invalid event type");
    }
}

/**
 * @brief the main logic function
 * at first, checking input validity
 * then, create Club object and try to execute event_string
 * at the end, prints output if all is fine
 * @param input_file 
 */
void ReadData(const std::string& input_file) {
    std::ifstream inputFile(input_file);
    if (!inputFile.is_open()) {
        throw std::runtime_error("Error: could not open input file");
    }

    // read number of tables
    int num_tables;
    std::string input_line;
    if (!std::getline(inputFile, input_line)) {
        throw std::runtime_error("Error at line 1: invalid input format");
    }
    std::istringstream is(input_line);
    if (!(is >> num_tables)) {
        throw std::runtime_error("Error at line 1: invalid number of tables");
    }
    if (num_tables <= 0) {
        throw std::runtime_error("Error at line 1: number of tables must be positive");
    }

    // read open and close time
    if (!std::getline(inputFile, input_line)) {
        throw std::runtime_error("Error at line 2: invalid input format");
    }
    std::pair<int, int> schedule = ReadInputTime(input_line);

    // read hourly rate
    int hourly_rate;
    if (!std::getline(inputFile, input_line)) {
        throw std::runtime_error("Error at line 3: invalid input format");
    }
    is.clear();
    is.str(input_line);
    if (!(is >> hourly_rate)) {
        throw std::runtime_error("Error at line 3: invalid hourly rate");
    }
    if (hourly_rate <= 0) {
        throw std::runtime_error("Error at line 3: hourly rate must be positive");
    }

    ComputerClub club(num_tables, schedule, hourly_rate);

    // read events
    std::string eventData;
    int k = 4;
    while (std::getline(inputFile, eventData)) {
        if (!CheckEventString(club, eventData))
            throw std::runtime_error("Error at line " + std::to_string(k) + ": invalid event format");
        
        std::shared_ptr<Event> event = MakeEvent(eventData);
        club.processEvent(event, eventData);
        k++;
    }
    club.ForcedLeaving(1);
    for (size_t i = 0; i < club.num_tables_; ++i) {
        std::cout << i + 1 << " " << club.revenue_table[i] << " " << club.IntTimeToString(club.working_table_time[i]) << "\n";
    }
}

int main(int argc, char *argv[]) {
    try {
        std::string input_file = argv[1];
        ReadData(input_file);
    } catch (const std::runtime_error& e) {
        // was catched runtime exception, which means that input data is incorrect
        std::cout << e.what();
    }
}