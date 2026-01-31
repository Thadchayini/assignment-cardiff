/*
  Eco-Clean Laundry Service - Menu Driven System (C++)
  ---------------------------------------------------
  Features:
   - Login / Logout
   - View service packages
   - Customer management: Add / Update / Delete / List
   - Order management: Place order / Update status / View history / Generate bill
   - Company details
   - File-based storage + simple backup files
   - Input validation + user-friendly navigation

  Storage files (created automatically if missing):
    users.txt      -> username password (one per line)
    packages.txt   -> id|name|unitType|rate
    customers.txt  -> id|name|phone|address
    orders.txt     -> orderId|customerId|packageId|qty|status|dateTime|amount

  Note: If users.txt is missing, program creates default user:
        admin admin123
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <ctime>
#include <limits>
#include <algorithm>

using namespace std;

// --------------------------- Constants / Filenames ---------------------------
const string USERS_FILE     = "users.txt";
const string PACKAGES_FILE  = "packages.txt";
const string CUSTOMERS_FILE = "customers.txt";
const string ORDERS_FILE    = "orders.txt";

// backups
const string USERS_BAK      = "users.bak.txt";
const string PACKAGES_BAK   = "packages.bak.txt";
const string CUSTOMERS_BAK  = "customers.bak.txt";
const string ORDERS_BAK     = "orders.bak.txt";

// --------------------------- Data Models -----------------------------------
struct Package {
    int id{};
    string name;
    string unitType; // "KG" or "PCS"
    double rate{};   // price per unit
};

struct Customer {
    int id{};
    string name;
    string phone;
    string address;
};

struct Order {
    int orderId{};
    int customerId{};
    int packageId{};
    double qty{};        // kg or pieces
    string status;       // Placed/In Progress/Ready/Collected
    string dateTime;     // simple string timestamp
    double amount{};     // computed
};

// --------------------------- Utility Helpers --------------------------------
static inline string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

string nowDateTime() {
    time_t t = time(nullptr);
    tm *lt = localtime(&t);
    ostringstream oss;
    oss << put_time(lt, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void pressEnter() {
    cout << "\nPress ENTER to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int readInt(const string& prompt, int minV, int maxV) {
    while (true) {
        cout << prompt;
        int x;
        if (cin >> x) {
            clearInput();
            if (x >= minV && x <= maxV) return x;
            cout << " Please enter a number between " << minV << " and " << maxV << ".\n";
        } else {
            cout << " Invalid input. Please enter a number.\n";
            clearInput();
        }
    }
}

double readDouble(const string& prompt, double minV) {
    while (true) {
        cout << prompt;
        double x;
        if (cin >> x) {
            clearInput();
            if (x >= minV) return x;
            cout << " Please enter a value >= " << minV << ".\n";
        } else {
            cout << " Invalid input. Please enter a numeric value.\n";
            clearInput();
        }
    }
}

string readLineNonEmpty(const string& prompt) {
    while (true) {
        cout << prompt;
        string s;
        getline(cin, s);
        s = trim(s);
        if (!s.empty()) return s;
        cout << " Cannot be empty. Try again.\n";
    }
}

bool confirmYesNo(const string& prompt) {
    while (true) {
        cout << prompt << " (y/n): ";
        string s;
        getline(cin, s);
        s = trim(s);
        if (s == "y" || s == "Y") return true;
        if (s == "n" || s == "N") return false;
        cout << " Please type y or n.\n";
    }
}

void backupFile(const string& src, const string& bak) {
    ifstream in(src, ios::binary);
    if (!in.good()) return;
    ofstream out(bak, ios::binary);
    out << in.rdbuf();
}

bool ensureFileExistsWithDefaults() {
    // users.txt
    {
        ifstream in(USERS_FILE);
        if (!in.good()) {
            ofstream out(USERS_FILE);
            out << "admin admin123\n";
        }
    }

    // packages.txt
    {
        ifstream in(PACKAGES_FILE);
        if (!in.good()) {
            ofstream out(PACKAGES_FILE);
            // id|name|unitType|rate
            out << "1|Wash & Fold|KG|450\n";
            out << "2|Dry Cleaning|PCS|300\n";
            out << "3|Ironing|PCS|150\n";
        }
    }

    // customers.txt, orders.txt
    {
        ifstream in(CUSTOMERS_FILE);
        if (!in.good()) {
            ofstream out(CUSTOMERS_FILE);
        }
    }
    {
        ifstream in(ORDERS_FILE);
        if (!in.good()) {
            ofstream out(ORDERS_FILE);
        }
    }

    return true;
}

// --------------------------- File Parsing / Loading --------------------------
vector<string> splitBy(const string& line, char delim) {
    vector<string> parts;
    string token;
    stringstream ss(line);
    while (getline(ss, token, delim)) parts.push_back(token);
    return parts;
}

vector<Package> loadPackages() {
    vector<Package> list;
    ifstream in(PACKAGES_FILE);
    string line;
    while (getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        auto parts = splitBy(line, '|');
        if (parts.size() != 4) continue;
        Package p;
        p.id = stoi(parts[0]);
        p.name = parts[1];
        p.unitType = parts[2];
        p.rate = stod(parts[3]);
        list.push_back(p);
    }
    return list;
}

vector<Customer> loadCustomers() {
    vector<Customer> list;
    ifstream in(CUSTOMERS_FILE);
    string line;
    while (getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        auto parts = splitBy(line, '|');
        if (parts.size() != 4) continue;
        Customer c;
        c.id = stoi(parts[0]);
        c.name = parts[1];
        c.phone = parts[2];
        c.address = parts[3];
        list.push_back(c);
    }
    return list;
}

vector<Order> loadOrders() {
    vector<Order> list;
    ifstream in(ORDERS_FILE);
    string line;
    while (getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        auto parts = splitBy(line, '|');
        if (parts.size() != 7) continue;
        Order o;
        o.orderId = stoi(parts[0]);
        o.customerId = stoi(parts[1]);
        o.packageId = stoi(parts[2]);
        o.qty = stod(parts[3]);
        o.status = parts[4];
        o.dateTime = parts[5];
        o.amount = stod(parts[6]);
        list.push_back(o);
    }
    return list;
}

void saveCustomers(const vector<Customer>& list) {
    backupFile(CUSTOMERS_FILE, CUSTOMERS_BAK);
    ofstream out(CUSTOMERS_FILE);
    for (auto &c : list) {
        out << c.id << "|" << c.name << "|" << c.phone << "|" << c.address << "\n";
    }
}

void saveOrders(const vector<Order>& list) {
    backupFile(ORDERS_FILE, ORDERS_BAK);
    ofstream out(ORDERS_FILE);
    for (auto &o : list) {
        out << o.orderId << "|" << o.customerId << "|" << o.packageId << "|"
            << o.qty << "|" << o.status << "|" << o.dateTime << "|" << o.amount << "\n";
    }
}

// --------------------------- Find Helpers -----------------------------------
Package* findPackageById(vector<Package>& pkgs, int id) {
    for (auto &p : pkgs) if (p.id == id) return &p;
    return nullptr;
}

Customer* findCustomerById(vector<Customer>& customers, int id) {
    for (auto &c : customers) if (c.id == id) return &c;
    return nullptr;
}

Order* findOrderById(vector<Order>& orders, int id) {
    for (auto &o : orders) if (o.orderId == id) return &o;
    return nullptr;
}

int nextCustomerId(const vector<Customer>& customers) {
    int mx = 0;
    for (auto &c : customers) mx = max(mx, c.id);
    return mx + 1;
}

int nextOrderId(const vector<Order>& orders) {
    int mx = 0;
    for (auto &o : orders) mx = max(mx, o.orderId);
    return mx + 1;
}

// --------------------------- Display Helpers --------------------------------
void showHeader(const string& title) {
    cout << "\n========================================\n";
    cout << "   " << title << "\n";
    cout << "========================================\n";
}

void viewCompanyDetails() {
    showHeader("Eco-Clean Laundry Service - Company Details");
    cout << "Company : Eco-Clean Laundry Service\n";
    cout << "Services: Wash & Fold, Dry Cleaning, Ironing\n";
    cout << "System  : Automated Order & Billing Management\n";
    cout << "Note    : This is a menu-driven demo system using file storage.\n";
}

// --------------------------- Authentication ---------------------------------
bool checkUserCredentials(const string& username, const string& password) {
    ifstream in(USERS_FILE);
    string u, p;
    while (in >> u >> p) {
        if (u == username && p == password) return true;
    }
    return false;
}

bool loginModule(string &loggedUser) {
    showHeader("Login");
    const int MAX_ATTEMPTS = 3;

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
        string username, password;

        cout << "Username: ";
        getline(cin, username);
        username = trim(username);

        cout << "Password: ";
        getline(cin, password);
        password = trim(password);

        if (checkUserCredentials(username, password)) {
            loggedUser = username;
            cout << "\n✅ Login successful. Welcome, " << loggedUser << "!\n";
            return true;
        } else {
            cout << "\n Invalid username or password.\n";
            cout << "Attempts left: " << (MAX_ATTEMPTS - attempt) << "\n\n";
        }
    }

    cout << " Too many failed attempts. Exiting...\n";
    return false;
}

// --------------------------- Packages Module --------------------------------
void viewPackagesModule(const vector<Package>& pkgs) {
    showHeader("Available Service Packages");
    cout << left << setw(6) << "ID"
         << setw(20) << "Package"
         << setw(8) << "Unit"
         << setw(10) << "Rate"
         << "\n";
    cout << "----------------------------------------\n";
    for (auto &p : pkgs) {
        cout << left << setw(6) << p.id
             << setw(20) << p.name
             << setw(8) << p.unitType
             << "LKR " << fixed << setprecision(2) << p.rate << "\n";
    }
}

// --------------------------- Customer Module --------------------------------
void listCustomers(const vector<Customer>& customers) {
    showHeader("Customer List");
    if (customers.empty()) {
        cout << "No customers found.\n";
        return;
    }
    cout << left << setw(6) << "ID"
         << setw(22) << "Name"
         << setw(15) << "Phone"
         << "Address\n";
    cout << "----------------------------------------\n";
    for (auto &c : customers) {
        cout << left << setw(6) << c.id
             << setw(22) << c.name
             << setw(15) << c.phone
             << c.address << "\n";
    }
}

void addCustomer(vector<Customer>& customers) {
    showHeader("Add Customer");
    string name = readLineNonEmpty("Customer Name   : ");
    string phone = readLineNonEmpty("Phone Number    : ");
    string address = readLineNonEmpty("Address         : ");

    Customer c;
    c.id = nextCustomerId(customers);
    c.name = name;
    c.phone = phone;
    c.address = address;

    customers.push_back(c);
    saveCustomers(customers);

    cout << "\n✅ Customer added successfully! Customer ID: " << c.id << "\n";
}

void updateCustomer(vector<Customer>& customers) {
    showHeader("Update Customer");
    if (customers.empty()) {
        cout << "No customers available.\n";
        return;
    }

    int id = readInt("Enter Customer ID to update: ", 1, 1000000000);
    Customer* c = findCustomerById(customers, id);
    if (!c) {
        cout << " Customer not found.\n";
        return;
    }

    cout << "\nCurrent Details:\n";
    cout << "Name   : " << c->name << "\n";
    cout << "Phone  : " << c->phone << "\n";
    cout << "Address: " << c->address << "\n\n";

    cout << "Enter new details (leave blank to keep current).\n";

    cout << "New Name    : ";
    string name; getline(cin, name); name = trim(name);
    cout << "New Phone   : ";
    string phone; getline(cin, phone); phone = trim(phone);
    cout << "New Address : ";
    string address; getline(cin, address); address = trim(address);

    if (!name.empty()) c->name = name;
    if (!phone.empty()) c->phone = phone;
    if (!address.empty()) c->address = address;

    saveCustomers(customers);
    cout << "\n✅ Customer updated successfully.\n";
}

void deleteCustomer(vector<Customer>& customers, vector<Order>& orders) {
    showHeader("Delete Customer");
    if (customers.empty()) {
        cout << "No customers available.\n";
        return;
    }

    int id = readInt("Enter Customer ID to delete: ", 1, 1000000000);
    Customer* c = findCustomerById(customers, id);
    if (!c) {
        cout << " Customer not found.\n";
        return;
    }

    // Check if customer has orders
    bool hasOrders = false;
    for (auto &o : orders) {
        if (o.customerId == id) {
            hasOrders = true;
            break;
        }
    }
    if (hasOrders) {
        cout << " Cannot delete customer because they have order history.\n";
        cout << "   Tip: Keep customers for record accuracy.\n";
        return;
    }

    if (!confirmYesNo("Are you sure you want to delete this customer")) {
        cout << "Cancelled.\n";
        return;
    }

    customers.erase(remove_if(customers.begin(), customers.end(),
                              [id](const Customer& x){ return x.id == id; }),
                    customers.end());

    saveCustomers(customers);
    cout << "✅ Customer deleted successfully.\n";
}

void customerManagementMenu(vector<Customer>& customers, vector<Order>& orders) {
    while (true) {
        showHeader("Customer Management");
        cout << "1. Add Customer\n";
        cout << "2. Update Customer\n";
        cout << "3. Delete Customer\n";
        cout << "4. View Customer List\n";
        cout << "5. Back to Main Menu\n";

        int choice = readInt("Choose an option: ", 1, 5);

        switch (choice) {
            case 1: addCustomer(customers); break;
            case 2: updateCustomer(customers); break;
            case 3: deleteCustomer(customers, orders); break;
            case 4: listCustomers(customers); break;
            case 5: return;
        }
        pressEnter();
    }
}

// --------------------------- Order Module -----------------------------------
void listOrders(const vector<Order>& orders, const vector<Customer>& customers, const vector<Package>& pkgs) {
    showHeader("Order History (All Orders)");
    if (orders.empty()) {
        cout << "No orders found.\n";
        return;
    }

    cout << left << setw(8) << "OrderID"
         << setw(10) << "CustID"
         << setw(18) << "Customer"
         << setw(16) << "Package"
         << setw(8) << "Qty"
         << setw(14) << "Status"
         << setw(20) << "DateTime"
         << "Amount\n";
    cout << "--------------------------------------------------------------------------------------------------\n";

    for (auto &o : orders) {
        string custName = "Unknown";
        for (auto &c : customers) if (c.id == o.customerId) { custName = c.name; break; }

        string pkgName = "Unknown";
        for (auto &p : pkgs) if (p.id == o.packageId) { pkgName = p.name; break; }

        cout << left << setw(8) << o.orderId
             << setw(10) << o.customerId
             << setw(18) << custName.substr(0,17)
             << setw(16) << pkgName.substr(0,15)
             << setw(8) << fixed << setprecision(2) << o.qty
             << setw(14) << o.status
             << setw(20) << o.dateTime
             << "LKR " << fixed << setprecision(2) << o.amount << "\n";
    }
}

double computeBill(const Package& pkg, double qty) {
    return pkg.rate * qty;
}

void placeNewOrder(vector<Order>& orders, vector<Customer>& customers, vector<Package>& pkgs) {
    showHeader("Place New Order");

    if (customers.empty()) {
        cout << " No customers available. Please add a customer first.\n";
        return;
    }

    int custId = readInt("Enter Customer ID: ", 1, 1000000000);
    Customer* cust = findCustomerById(customers, custId);
    if (!cust) {
        cout << " Customer not found.\n";
        return;
    }

    viewPackagesModule(pkgs);
    int pkgId = readInt("\nSelect Package ID: ", 1, 1000000000);
    Package* pkg = findPackageById(pkgs, pkgId);
    if (!pkg) {
        cout << " Invalid package selected.\n";
        return;
    }

    double qty = 0;
    if (pkg->unitType == "KG") {
        qty = readDouble("Enter weight (KG): ", 0.1);
    } else {
        qty = readDouble("Enter number of pieces: ", 1);
    }

    double amount = computeBill(*pkg, qty);

    Order o;
    o.orderId = nextOrderId(orders);
    o.customerId = custId;
    o.packageId = pkgId;
    o.qty = qty;
    o.status = "Placed";
    o.dateTime = nowDateTime();
    o.amount = amount;

    orders.push_back(o);
    saveOrders(orders);

    cout << "\n✅ Order placed successfully!\n";
    cout << "Order ID : " << o.orderId << "\n";
    cout << "Customer : " << cust->name << " (ID " << cust->id << ")\n";
    cout << "Package  : " << pkg->name << "\n";
    cout << "Qty      : " << fixed << setprecision(2) << o.qty << " " << pkg->unitType << "\n";
    cout << "Amount   : LKR " << fixed << setprecision(2) << o.amount << "\n";
}

void updateOrderStatus(vector<Order>& orders) {
    showHeader("Update Order Status");

    if (orders.empty()) {
        cout << "No orders available.\n";
        return;
    }

    int oid = readInt("Enter Order ID: ", 1, 1000000000);
    Order* o = findOrderById(orders, oid);
    if (!o) {
        cout << " Order not found.\n";
        return;
    }

    cout << "\nCurrent Status: " << o->status << "\n";
    cout << "1. Placed\n";
    cout << "2. In Progress\n";
    cout << "3. Ready\n";
    cout << "4. Collected\n";
    int c = readInt("Choose new status: ", 1, 4);

    switch (c) {
        case 1: o->status = "Placed"; break;
        case 2: o->status = "In Progress"; break;
        case 3: o->status = "Ready"; break;
        case 4: o->status = "Collected"; break;
    }

    saveOrders(orders);
    cout << "✅ Status updated successfully.\n";
}

void viewCustomerOrderHistory(const vector<Order>& orders, int custId, const vector<Package>& pkgs) {
    showHeader("Customer Order History");

    bool any = false;
    for (auto &o : orders) {
        if (o.customerId == custId) {
            any = true;
            string pkgName = "Unknown";
            string unit = "";
            for (auto &p : pkgs) if (p.id == o.packageId) { pkgName = p.name; unit = p.unitType; break; }

            cout << "----------------------------------------\n";
            cout << "Order ID : " << o.orderId << "\n";
            cout << "Package  : " << pkgName << "\n";
            cout << "Qty      : " << fixed << setprecision(2) << o.qty << " " << unit << "\n";
            cout << "Status   : " << o.status << "\n";
            cout << "DateTime : " << o.dateTime << "\n";
            cout << "Amount   : LKR " << fixed << setprecision(2) << o.amount << "\n";
        }
    }

    if (!any) {
        cout << "No orders found for this customer.\n";
    }
}

void orderHistoryModule(vector<Order>& orders, vector<Customer>& customers, vector<Package>& pkgs) {
    showHeader("View Order History");
    cout << "1. View All Orders\n";
    cout << "2. View Orders by Customer ID\n";
    cout << "3. Back\n";
    int choice = readInt("Choose an option: ", 1, 3);

    if (choice == 1) {
        listOrders(orders, customers, pkgs);
    } else if (choice == 2) {
        int custId = readInt("Enter Customer ID: ", 1, 1000000000);
        Customer* cust = findCustomerById(customers, custId);
        if (!cust) {
            cout << " Customer not found.\n";
            return;
        }
        cout << "\nCustomer: " << cust->name << "\n";
        viewCustomerOrderHistory(orders, custId, pkgs);
    } else {
        return;
    }
}

void generateBillModule(vector<Order>& orders, vector<Customer>& customers, vector<Package>& pkgs) {
    showHeader("Generate Bill");
    if (orders.empty()) {
        cout << "No orders available.\n";
        return;
    }

    int oid = readInt("Enter Order ID: ", 1, 1000000000);
    Order* o = findOrderById(orders, oid);
    if (!o) {
        cout << " Order not found.\n";
        return;
    }

    Customer* cust = findCustomerById(customers, o->customerId);
    Package* pkg = findPackageById(pkgs, o->packageId);

    cout << "\n=========== ECO-CLEAN BILL ===========\n";
    cout << "Bill Date : " << nowDateTime() << "\n";
    cout << "Order ID  : " << o->orderId << "\n";
    cout << "Customer  : " << (cust ? cust->name : "Unknown") << " (ID " << o->customerId << ")\n";
    cout << "Package   : " << (pkg ? pkg->name : "Unknown") << "\n";
    cout << "Qty       : " << fixed << setprecision(2) << o->qty << " " << (pkg ? pkg->unitType : "") << "\n";
    cout << "Status    : " << o->status << "\n";
    cout << "-------------------------------------\n";
    cout << "TOTAL     : LKR " << fixed << setprecision(2) << o->amount << "\n";
    cout << "=====================================\n";

    if (o->status != "Collected") {
        if (confirmYesNo("Mark this order as Collected")) {
            o->status = "Collected";
            saveOrders(orders);
            cout << "✅ Order marked as Collected.\n";
        }
    } else {
        cout << "Note: Order already collected.\n";
    }
}

void orderManagementMenu(vector<Order>& orders, vector<Customer>& customers, vector<Package>& pkgs) {
    while (true) {
        showHeader("Order Management");
        cout << "1. Place New Order\n";
        cout << "2. Update Order Status\n";
        cout << "3. View Order History\n";
        cout << "4. Generate Bill\n";
        cout << "5. Back to Main Menu\n";

        int choice = readInt("Choose an option: ", 1, 5);

        switch (choice) {
            case 1: placeNewOrder(orders, customers, pkgs); break;
            case 2: updateOrderStatus(orders); break;
            case 3: orderHistoryModule(orders, customers, pkgs); break;
            case 4: generateBillModule(orders, customers, pkgs); break;
            case 5: return;
        }
        pressEnter();
    }
}

// --------------------------- Main Menu --------------------------------------
int mainMenu(const string& loggedUser,
             vector<Package>& pkgs,
             vector<Customer>& customers,
             vector<Order>& orders) {

    while (true) {
        showHeader("Eco-Clean Laundry System - Main Menu");
        cout << "Logged in as: " << loggedUser << "\n\n";
        cout << "1. View Service Packages\n";
        cout << "2. Customer Management\n";
        cout << "3. Order Management\n";
        cout << "4. View Company Details\n";
        cout << "5. Logout\n";
        cout << "6. Exit Application\n";

        int choice = readInt("Choose an option: ", 1, 6);

        switch (choice) {
            case 1:
                viewPackagesModule(pkgs);
                pressEnter();
                break;
            case 2:
                customerManagementMenu(customers, orders);
                break;
            case 3:
                orderManagementMenu(orders, customers, pkgs);
                break;
            case 4:
                viewCompanyDetails();
                pressEnter();
                break;
            case 5:
                cout << "\n✅ Logged out.\n";
                return 1; // logout
            case 6:
                cout << "\n✅ Exiting application...\n";
                return 0; // exit
        }
    }
}

// --------------------------- Program Entry ----------------------------------
int main() {
    ios::sync_with_stdio(false);
    // cin.tie(nullptr);

    ensureFileExistsWithDefaults();

    // Backup critical files on startup (simple backup requirement)
    backupFile(USERS_FILE, USERS_BAK);
    backupFile(PACKAGES_FILE, PACKAGES_BAK);
    backupFile(CUSTOMERS_FILE, CUSTOMERS_BAK);
    backupFile(ORDERS_FILE, ORDERS_BAK);

    // Load data
    vector<Package> pkgs = loadPackages();
    vector<Customer> customers = loadCustomers();
    vector<Order> orders = loadOrders();

    while (true) {
        string loggedUser;
        bool ok = loginModule(loggedUser);
        if (!ok) return 0;

        int result = mainMenu(loggedUser, pkgs, customers, orders);
        if (result == 0) break; // exit
        // if logout, loop back to login
    }

    return 0;
}
