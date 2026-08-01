#include <drogon/drogon.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>

using namespace drogon;

// بيانات الاتصال بقاعدة بيانات Supabase
const std::string SUPABASE_URL = "https://pxwlzbxfnzbijazwtfti.supabase.co";
const std::string SUPABASE_KEY = "sb_publishable_6LLbnGvedqGuIJrq-UaFJA_DL78835B";

// دالة تحويل النصوص للحروف الصغيرة لضمان مرونة الفلترة
std::string toLowerStr(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

int main() {
    int port = 8080;
    if (const char* env_p = std::getenv("PORT")) {
        port = std::stoi(env_p);
    }

    std::cout << "🚀 Running Drogon C++ Server on Render, Port: " << port << std::endl;

    // 1. تفعيل CORS
    app().registerSyncAdvice([](const HttpRequestPtr &req) -> HttpResponsePtr {
        if (req->method() == Options) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, apikey");
            return resp;
        }
        return nullptr;
    });

    app().registerPostHandlingAdvice([](const HttpRequestPtr &req, const HttpResponsePtr &resp) {
        if (resp) {
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, apikey");
        }
    });

    // 2. API - جلب وتصفية المناجم
    app().registerHandler("/api/mines", [](const HttpRequestPtr &req,
                                            std::function<void(const HttpResponsePtr &)> &&callback) {
        auto client = HttpClient::newHttpClient(SUPABASE_URL);
        auto supabaseReq = HttpRequest::newHttpRequest();

        supabaseReq->setPath("/rest/v1/mines?select=*");
        supabaseReq->setMethod(Get);
        supabaseReq->addHeader("apikey", SUPABASE_KEY);
        supabaseReq->addHeader("Authorization", "Bearer " + SUPABASE_KEY);

        std::string mineral = toLowerStr(req->getParameter("mineral"));
        std::string company = toLowerStr(req->getParameter("company"));
        std::string province = toLowerStr(req->getParameter("province"));
        std::string minProdStr = req->getParameter("min_prod");
        double minProd = minProdStr.empty() ? 0.0 : std::stod(minProdStr);

        client->sendRequest(supabaseReq, [callback, mineral, company, province, minProd](ReqResult result, const HttpResponsePtr &response) {
            // استخدام getJsonObject() وهي الدالة الصحيحة والموجودة في Drogon
            if (result != ReqResult::Ok || !response || !response->getJsonObject()) {
                auto errResp = HttpResponse::newHttpJsonResponse(Json::Value(Json::arrayValue));
                errResp->setStatusCode(k500InternalServerError);
                callback(errResp);
                return;
            }

            // getJsonObject يُرجع shared_ptr<const Json::Value>
            const Json::Value &rawMines = *(response->getJsonObject());
            Json::Value filteredMines(Json::arrayValue);

            if (rawMines.isArray()) {
                for (const auto &mine : rawMines) {
                    std::string mMineral = "";
                    if (mine.isMember("primary_mineral") && !mine["primary_mineral"].isNull()) {
                        mMineral = toLowerStr(mine["primary_mineral"].asString());
                    }

                    std::string mCompany = "";
                    if (mine.isMember("operator") && !mine["operator"].isNull()) {
                        mCompany = toLowerStr(mine["operator"].asString());
                    }

                    std::string mProvince = "";
                    if (mine.isMember("province") && !mine["province"].isNull()) {
                        mProvince = toLowerStr(mine["province"].asString());
                    }

                    double prodVal = 0.0;
                    if (mine.isMember("monthly_production_tons") && !mine["monthly_production_tons"].isNull() && mine["monthly_production_tons"].isNumeric()) {
                        prodVal = mine["monthly_production_tons"].asDouble();
                    }

                    // مطابقة مرنة للفلترة
                    bool matchMineral = (mineral.empty() || mineral == "all" || mMineral.find(mineral) != std::string::npos);
                    bool matchCompany = (company.empty() || company == "all" || mCompany.find(company) != std::string::npos);
                    bool matchProvince = (province.empty() || province == "all" || mProvince == province);
                    bool matchProd = (prodVal >= minProd);

                    if (matchMineral && matchCompany && matchProvince && matchProd) {
                        filteredMines.append(mine);
                    }
                }
            }

            auto res = HttpResponse::newHttpJsonResponse(filteredMines);
            callback(res);
        });
    }, {Get});

    // 3. تشغيل السيرفر
    app().setDocumentRoot("./public")
         .addListener("0.0.0.0", port)
         .setThreadNum(2)
         .run();

    return 0;
}
