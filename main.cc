#include <drogon/drogon.h>
#include <json/json.h>
#include <iostream>
#include <cstdlib>

using namespace drogon;

// ============================================================================
// 1. بيانات الاتصال بـ Supabase الخاص بك
// ============================================================================
const std::string SUPABASE_URL = "https://pxwlzbxfnzbijazwtfti.supabase.co";
const std::string SUPABASE_KEY = "sb_publishable_6LLbnGvedqGuIJrq-UaFJA_DL78835B";

int main() {
    // ============================================================================
    // 2. تسجيل مسار الـ API لعرض المناجم والتصفية
    // ============================================================================
    app().registerHandler("/api/mines", [](const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
        
        // معالجة طلبات Preflight الخاصّة بـ CORS (OPTIONS)
        if (req->method() == Options) {
            auto res = HttpResponse::newHttpResponse();
            res->setStatusCode(k200OK);
            res->addHeader("Access-Control-Allow-Origin", "*");
            res->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            res->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, apikey");
            callback(res);
            return;
        }

        // قراءة معلمات التصفية (Query Parameters)
        auto mineral = req->getParameter("mineral");
        auto company = req->getParameter("company");
        auto province = req->getParameter("province");
        auto minProd = req->getParameter("min_prod");

        // إنشاء عميل HTTP للاتصال بـ Supabase
        auto client = HttpClient::newHttpClient(SUPABASE_URL);
        
        // بناء مسار الاستعلام
        std::string path = "/rest/v1/mines?select=*";

        if (!mineral.empty() && mineral != "all") {
            path += "&primary_mineral=eq." + mineral;
        }
        if (!company.empty() && company != "all") {
            path += "&operator=eq." + company;
        }
        if (!province.empty() && province != "all") {
            path += "&province=eq." + province;
        }
        if (!minProd.empty() && minProd != "0") {
            path += "&monthly_production_tons=gte." + minProd;
        }

        auto supabaseReq = HttpRequest::newHttpRequest();
        supabaseReq->setPath(path);
        supabaseReq->setMethod(drogon::Get);

        // إرسال مفاتيح الهيدر الصحيحة لـ Supabase لفك التشفير وإعادة البيانات
        supabaseReq->addHeader("apikey", SUPABASE_KEY);
        supabaseReq->addHeader("Authorization", "Bearer " + SUPABASE_KEY);
        supabaseReq->addHeader("Accept", "application/json");

        // إرسال الطلب واستلام البيانات
        client->sendRequest(supabaseReq, [callback](ReqResult result, const HttpResponsePtr &response) {
            auto res = HttpResponse::newHttpResponse();
            
            // إضافة هيدرز CORS في جميع الحالات
            res->addHeader("Access-Control-Allow-Origin", "*");
            res->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            res->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, apikey");

            if (result == ReqResult::Ok && response && response->getStatusCode() == k200OK) {
                res->setStatusCode(k200OK);
                res->setContentTypeCode(CT_APPLICATION_JSON);
                res->setBody(std::string(response->getBody()));
            } else {
                // في حالة وجود خطأ يتم إرجاع مصفوفة فارغة مغلّفة بنجاح
                res->setStatusCode(k500InternalServerError);
                res->setContentTypeCode(CT_APPLICATION_JSON);
                res->setBody("[]");
            }
            callback(res);
        });
    }, {Get, Options});

    // ============================================================================
    // 3. تحديد المنفذ وتشغيل السيرفر على Render
    // ============================================================================
    int port = 10000;
    if (const char* envPort = std::getenv("PORT")) {
        port = std::atoi(envPort);
    }

    std::cout << "Running Drogon C++ Server on Render, Port: " << port << std::endl;
    
    app().addListener("0.0.0.0", port);
    app().run();

    return 0;
}
