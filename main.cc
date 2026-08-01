#include <drogon/drogon.h>
#include <iostream>
#include <string>
#include <cstdlib>

using namespace drogon;

// بيانات الاتصال بقاعدة بيانات Supabase
const std::string SUPABASE_URL = "https://pxwlzbxfnzbijazwtfti.supabase.co";
const std::string SUPABASE_KEY = "sb_publishable_6LLbnGvedqGuIJrq-UaFJA_DL78835B";

int main() {
    // تحديد المنفذ (Port) تلقائياً لدعم الاستضافة السحابية
    int port = 8080;
    if (const char* env_p = std::getenv("PORT")) {
        port = std::stoi(env_p);
    }

    std::cout << "🚀 Running Drogon C++ Server on port: " << port << std::endl;

    // 1. تفعيل إعدادات CORS على مستوى جميع الطلبات والردود
    // أ) إضافة الترويسات (Headers) لكافة الردود التي يرسلها السيرفر
    app().registerPostRoutingAdvice([](const HttpRequestPtr &req, AdviceCallback &&ac, AdviceChainCallback &&acc) {
        // ننتقل للتنفيذ التالي في السلسلة وحين نحصل على الرد نضيف رؤوس CORS
        acc();
    });

    // ب) استخدام SyncAdvice لمعالجة طلبات الـ Preflight (OPTIONS) بسرعة وسهولة
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

    // 2. معالج عام لإضافة رؤوس CORS على أي استجابة صادرة من السيرفر
    app().registerPostHandlingAdvice([](const HttpRequestPtr &req, const HttpResponsePtr &resp) {
        if (resp) {
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, apikey");
        }
    });

    // 3. API - جلب بيانات المناجم مع تطبيق التصفية المتقدمة داخل C++
    app().registerHandler("/api/mines", [](const HttpRequestPtr &req,
                                            std::function<void(const HttpResponsePtr &)> &&callback) {
        auto client = HttpClient::newHttpClient(SUPABASE_URL);
        auto supabaseReq = HttpRequest::newHttpRequest();

        supabaseReq->setPath("/rest/v1/mines?select=*");
        supabaseReq->setMethod(Get);
        supabaseReq->addHeader("apikey", SUPABASE_KEY);
        supabaseReq->addHeader("Authorization", "Bearer " + SUPABASE_KEY);

        // استلام معاملات الفلترة من الواجهة
        std::string mineral = req->getParameter("mineral");
        std::string company = req->getParameter("company");
        std::string province = req->getParameter("province");
        std::string minProdStr = req->getParameter("min_prod");
        double minProd = minProdStr.empty() ? 0.0 : std::stod(minProdStr);

        client->sendRequest(supabaseReq, [callback, mineral, company, province, minProd](ReqResult result, const HttpResponsePtr &response) {
            if (result != ReqResult::Ok || !response || !response->getJsonObject()) {
                auto errResp = HttpResponse::newHttpJsonResponse(Json::Value(Json::arrayValue));
                errResp->setStatusCode(k500InternalServerError);
                callback(errResp);
                return;
            }

            const auto &rawMines = *response->getJsonObject();
            Json::Value filteredMines(Json::arrayValue);

            // معالجة وتصفية البيانات في الذاكرة لسرعة الأداء
            for (const auto &mine : rawMines) {
                bool matchMineral = mineral.empty() || mineral == "all" ||
                    (mine.isMember("primary_mineral") && mine["primary_mineral"].asString().find(mineral) != std::string::npos);

                bool matchCompany = company.empty() || company == "all" ||
                    (mine.isMember("operator") && mine["operator"].asString() == company);

                bool matchProvince = province.empty() || province == "all" ||
                    (mine.isMember("province") && mine["province"].asString() == province);

                double prodVal = (mine.isMember("monthly_production_tons") && mine["monthly_production_tons"].isNumeric()) ?
                                 mine["monthly_production_tons"].asDouble() : 0.0;
                
                bool matchProd = prodVal >= minProd;

                if (matchMineral && matchCompany && matchProvince && matchProd) {
                    filteredMines.append(mine);
                }
            }

            auto res = HttpResponse::newHttpJsonResponse(filteredMines);
            callback(res);
        });
    }, {Get});

    // 4. API - حساب الإحصائيات الاستراتيجية المتقدمة
    app().registerHandler("/api/stats", [](const HttpRequestPtr &req,
                                            std::function<void(const HttpResponsePtr &)> &&callback) {
        auto client = HttpClient::newHttpClient(SUPABASE_URL);
        auto supabaseReq = HttpRequest::newHttpRequest();

        supabaseReq->setPath("/rest/v1/mines?select=*");
        supabaseReq->setMethod(Get);
        supabaseReq->addHeader("apikey", SUPABASE_KEY);
        supabaseReq->addHeader("Authorization", "Bearer " + SUPABASE_KEY);

        client->sendRequest(supabaseReq, [callback](ReqResult result, const HttpResponsePtr &response) {
            if (result != ReqResult::Ok || !response || !response->getJsonObject()) {
                auto err = HttpResponse::newHttpJsonResponse(Json::Value(Json::objectValue));
                err->setStatusCode(k500InternalServerError);
                callback(err);
                return;
            }

            const auto &mines = *response->getJsonObject();
            double totalTons = 0.0;
            int count = 0;
            Json::Value companyBreakdown(Json::objectValue);

            for (const auto &mine : mines) {
                count++;
                if (mine.isMember("monthly_production_tons") && mine["monthly_production_tons"].isNumeric()) {
                    totalTons += mine["monthly_production_tons"].asDouble();
                }
                if (mine.isMember("operator")) {
                    std::string op = mine["operator"].asString();
                    companyBreakdown[op] = companyBreakdown[op].asInt() + 1;
                }
            }

            Json::Value stats;
            stats["active_mines"] = count;
            stats["total_monthly_tons"] = totalTons;
            stats["companies_breakdown"] = companyBreakdown;

            callback(HttpResponse::newHttpJsonResponse(stats));
        });
    }, {Get});

    // خدمة ملفات الواجهة الأمامية الثابتة من مجلد public
    app().setDocumentRoot("./public")
         .addListener("0.0.0.0", port)
         .setThreadNum(2)
         .run();

    return 0;
}
