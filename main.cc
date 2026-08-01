#include <drogon/drogon.h>
#include <json/json.h>
#include <iostream>
#include <cstdlib>

using namespace drogon;

// ============================================================================
// 1. بيانات الاتصال بـ Supabase الخاصة بمشروعك
// ============================================================================
const std::string SUPABASE_URL = "https://pxwlzbxfnzbijazwtfti.supabase.co";
const std::string SUPABASE_KEY = "sb_publishable_6LLbnGvedqGuIJrq-UaFJA_DL78835B";

int main() {
    // ============================================================================
    // 2. إعداد المسار API لجلـب المناجم مع معالجة كافة الهيدرز والفلاتر
    // ============================================================================
    app().registerHandler("/api/mines", [](const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
        
        // قراءة الفلاتر من الـ URL (إن وجدت)
        auto mineral = req->getParameter("mineral");
        auto company = req->getParameter("company");
        auto province = req->getParameter("province");
        auto minProd = req->getParameter("min_prod");

        // إنشاء عميل HTTP للاتصال بـ Supabase
        auto client = HttpClient::newHttpClient(SUPABASE_URL);
        
        // بناء المسار الموجه لـ Supabase REST API
        std::string path = "/rest/v1/mines?select=*";

        // إضافة الفلاتر تلقائياً للاستعلام
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

        // 🔑 إضافة الهيدرز المطلوبة لفك مشكلة الـ [] الفارغة
        supabaseReq->addHeader("apikey", SUPABASE_KEY);
        supabaseReq->addHeader("Authorization", "Bearer " + SUPABASE_KEY);
        supabaseReq->addHeader("Accept", "application/json");

        // إرسال الطلب لـ Supabase
        client->sendRequest(supabaseReq, [callback](ReqResult result, const HttpResponsePtr &response) {
            if (result == ReqResult::Ok && response && response->getStatusCode() == k200OK) {
                
                // إنشاء الرد وتمريره للواجهة الأمامية
                auto res = HttpResponse::newHttpResponse();
                res->setStatusCode(k200OK);
                res->setContentTypeCode(CT_APPLICATION_JSON);
                res->setBody(std::string(response->getBody()));

                // 🔓 السماح بالوصول عبر الـ CORS للواجهة
                res->addHeader("Access-Control-Allow-Origin", "*");
                res->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
                res->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, apikey");

                callback(res);
            } else {
                // في حال حدوث خطأ أثناء جلب البيانات
                auto res = HttpResponse::newJsonHttpResponse(Json::Value(Json::arrayValue));
                res->setStatusCode(k500InternalServerError);
                res->addHeader("Access-Control-Allow-Origin", "*");
                callback(res);
            }
        });
    }, {Get, Options});

    // ============================================================================
    // 3. معالجة طلبات الـ CORS Preflight (OPTIONS)
    // ============================================================================
    app().registerCorsHandling(
        "/*",
        [](const HttpRequestPtr &req) {
            return true;
        },
        [](const HttpRequestPtr &req, const HttpResponsePtr &resp) {
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, apikey");
        }
    );

    // ============================================================================
    // 4. إعداد المنفذ وتحديث السيرفر على Render
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
