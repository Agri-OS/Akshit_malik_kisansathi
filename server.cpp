#include "httplib.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>

using namespace std;

// URL mein space ko %20 banane wala function
string encodeSpaces(string str) {
    size_t pos = 0;
    while ((pos = str.find(" ", pos)) != string::npos) {
        str.replace(pos, 1, "%20");
        pos += 3; 
    }
    return str;
}

string executeCurlCommand(const string& url) {
    char buffer[128];
    string result = "";
    string cmd = "curl -g -s \"" + url + "\""; 
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "{\"error\": \"System curl execute nahi ho paya.\"}";
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) result += buffer;
    pclose(pipe);
    
    if(result.empty()) {
        return "{\"error\": \"Government API se data nahi mila.\"}";
    }
    return result;
}

string toLowerCase(string str) {
    transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

int getLevenshteinDistance(string s1, string s2) {
    s1 = toLowerCase(s1);
    s2 = toLowerCase(s2);
    int m = s1.length(), n = s2.length();
    vector<vector<int>> dp(m + 1, vector<int>(n + 1));

    for (int i = 0; i <= m; i++) {
        for (int j = 0; j <= n; j++) {
            if (i == 0) dp[i][j] = j;
            else if (j == 0) dp[i][j] = i;
            else if (s1[i - 1] == s2[j - 1])
                dp[i][j] = dp[i - 1][j - 1];
            else
                dp[i][j] = 1 + min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
        }
    }
    return dp[m][n];
}

// --- MEGA CROP DICTIONARY (Hinglish to English) ---
map<string, string> cropDictionary = {
    // Sabjiyan (Vegetables)
    {"onion", "Onion"}, {"pyaj", "Onion"}, {"pyaaz", "Onion"}, {"kanda", "Onion"},
    {"potato", "Potato"}, {"aloo", "Potato"}, {"aalu", "Potato"},
    {"tomato", "Tomato"}, {"tamatar", "Tomato"},
    {"garlic", "Garlic"}, {"lahsun", "Garlic"}, {"lasun", "Garlic"},
    {"cabbage", "Cabbage"}, {"patta gobhi", "Cabbage"}, {"band gobhi", "Cabbage"},
    {"cauliflower", "Cauliflower"}, {"phool gobhi", "Cauliflower"},
    {"carrot", "Carrot"}, {"gajar", "Carrot"},
    {"radish", "Radish"}, {"mooli", "Radish"},
    {"brinjal", "Brinjal"}, {"baingan", "Brinjal"}, {"eggplant", "Brinjal"},
    {"lady finger", "Bhindi"}, {"bhindi", "Bhindi"}, {"okra", "Bhindi"},
    {"bottle gourd", "Bottle Gourd"}, {"lauki", "Bottle Gourd"}, {"ghiya", "Bottle Gourd"},
    {"bitter gourd", "Bitter Gourd"}, {"karela", "Bitter Gourd"},
    {"spinach", "Spinach"}, {"palak", "Spinach"},
    {"capsicum", "Capsicum"}, {"shimla mirch", "Capsicum"},
    {"green chilli", "Green Chilli"}, {"hari mirch", "Green Chilli"},
    {"ginger", "Ginger"}, {"adrak", "Ginger"},
    {"coriander", "Coriander"}, {"dhaniya", "Coriander"},
    
    // Anaj aur Dalen (Grains & Pulses)
    {"wheat", "Wheat"}, {"gehu", "Wheat"}, {"gehun", "Wheat"},
    {"paddy", "Paddy"}, {"dhan", "Paddy"}, {"rice", "Paddy"}, {"chawal", "Paddy"},
    {"maize", "Maize"}, {"makka", "Maize"}, {"bhutta", "Maize"},
    {"mustard", "Mustard"}, {"sarso", "Mustard"}, {"sarson", "Mustard"},
    {"black gram", "Black Gram"}, {"urad", "Black Gram"}, {"urad dal", "Black Gram"},
    {"green gram", "Green Gram"}, {"moong", "Green Gram"}, {"moong dal", "Green Gram"},
    {"bengal gram", "Bengal Gram(Gram)"}, {"chana", "Bengal Gram(Gram)"}, {"chana dal", "Bengal Gram(Gram)"},
    {"red gram", "Arhar"}, {"arhar", "Arhar"}, {"toor", "Arhar"}, {"tuar", "Arhar"},
    {"soyabean", "Soyabean"}, {"soya", "Soyabean"},
    
    // Fal (Fruits)
    {"apple", "Apple"}, {"seb", "Apple"},
    {"banana", "Banana"}, {"kela", "Banana"},
    {"mango", "Mango"}, {"aam", "Mango"},
    {"papaya", "Papaya"}, {"papita", "Papaya"},
    {"pomegranate", "Pomegranate"}, {"anar", "Pomegranate"},
    {"grapes", "Grapes"}, {"angoor", "Grapes"},
    {"orange", "Orange"}, {"santra", "Orange"},
    {"guava", "Guava"}, {"amrood", "Guava"},
    {"watermelon", "Water Melon"}, {"tarbooj", "Water Melon"}
};

vector<string> getCropSuggestions(string input) {
    vector<string> suggestions;
    input = toLowerCase(input);
    for (const auto& pair : cropDictionary) {
        string keyword = pair.first; 
        if (keyword.find(input) == 0 || getLevenshteinDistance(input, keyword) <= 2) {
            suggestions.push_back(keyword);
        }
    }
    return suggestions;
}

string makeJsonArray(const vector<string>& list) {
    string json = "[";
    for (size_t i = 0; i < list.size(); ++i) {
        json += "\"" + list[i] + "\"";
        if (i < list.size() - 1) json += ", ";
    }
    json += "]";
    return json;
}

int main() {
    httplib::Server svr;

   // 1. SMART WEATHER API (GPS SUPPORT ADDED)
    svr.Get("/api/weather", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        httplib::Client weather_api("http://api.openweathermap.org");
        string weather_api_key = "8297848238a870691bf27d4ab453cf68"; 
        
        string path = "";
        
        // Agar GPS se Latitude aur Longitude aaya hai (Zyada accurate)
        if (req.has_param("lat") && req.has_param("lon")) {
            string lat = req.get_param_value("lat");
            string lon = req.get_param_value("lon");
            path = "/data/2.5/forecast?lat=" + lat + "&lon=" + lon + "&appid=" + weather_api_key + "&units=metric";
        } 
        // Varna Shehar ke naam se search karein
        else {
            string city = "Meerut";
            if (req.has_param("city") && req.get_param_value("city") != "") {
                city = req.get_param_value("city");
                city = encodeSpaces(city);
            }
            path = "/data/2.5/forecast?q=" + city + "&appid=" + weather_api_key + "&units=metric";
        }

        auto weather_res = weather_api.Get(path.c_str());
        
        if (weather_res && weather_res->status == 200) {
            res.set_content(weather_res->body, "application/json");
        } else {
            res.set_content("{\"error\": \"Kripya shehar ka sahi naam likhein ya GPS on karein.\"}", "application/json");
        }
    });

    // 2. LIVE MANDI API
    svr.Get("/api/mandi", [](const httplib::Request &req, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        string apiKey = "579b464db66ec23bdd000001705f5a08aa674b804cd0fdf49e7aa8d3"; 
        string url = "https://api.data.gov.in/resource/35985678-0d79-46b4-9ed6-6f13308a1d24?api-key=" + apiKey + "&format=json";

        if (req.has_param("commodity") && req.get_param_value("commodity") != "") {
            string commodity = req.get_param_value("commodity");
            commodity = toLowerCase(commodity);
            
            if (cropDictionary.find(commodity) != cropDictionary.end()) {
                commodity = cropDictionary[commodity]; 
            }
            if(commodity.length() > 0 && islower(commodity[0])) {
                commodity[0] = toupper(commodity[0]);
            }
            commodity = encodeSpaces(commodity);
            url += "&filters%5Bcommodity%5D=" + commodity; 
        }

        url += "&limit=2000"; 
        string jsonResponse = executeCurlCommand(url);
        res.set_content(jsonResponse, "application/json");
    });

    // 3. MANDI AUTO-SUGGEST API 
    svr.Get("/suggest", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*"); 
        if (req.has_param("q")) {
            string query = req.get_param_value("q");
            vector<string> result = getCropSuggestions(query);
            res.set_content(makeJsonArray(result), "application/json");
        } else {
            res.set_content("[]", "application/json");
        }
    });

    // 4. NEW: DYNAMIC CITY AUTO-SUGGEST (Hinglish/English Direct via OpenWeather)
    svr.Get("/suggest-city", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*"); 
        if (req.has_param("q")) {
            string query = req.get_param_value("q");
            query = encodeSpaces(query);
            
            httplib::Client weather_api("http://api.openweathermap.org");
            string weather_api_key = "8297848238a870691bf27d4ab453cf68"; 
            
            // Limit 5, IN = India priority
            string path = "/geo/1.0/direct?q=" + query + ",IN&limit=5&appid=" + weather_api_key;
            auto geo_res = weather_api.Get(path.c_str());
            
            if (geo_res && geo_res->status == 200) {
                // Seedhe JSON front-end ko bhej do, JS handle kar legi
                res.set_content(geo_res->body, "application/json");
            } else {
                res.set_content("[]", "application/json");
            }
        } else {
            res.set_content("[]", "application/json");
        }
    });

    cout << "===============================================" << endl;
    cout << "KisanSathi Smart Backend Server is Running..." << endl;
    cout << "Port: 8080 par listening chal rahi hai." << endl;
    cout << "===============================================" << endl;

    svr.listen("localhost", 8080);
    return 0;
}