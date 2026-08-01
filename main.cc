#include <drogon/drogon.h>
#include <json/json.h>
#include <iostream>
#include <cstdlib>

using namespace drogon;

// ============================================================================
// 1. بيانات الاتصال بـ Supabase
// ============================================================================
const std::string SUPABASE_URL = "https://pxwlzbxfnzbijazwtfti.supabase.co";
const std::string SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InB4d2x6YnhmbnpiaWphend0ZnRpIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODU0NDA2MjUsImV4cCI6MjEwMTAxNjYyNX0.KAWrEx5koLOVrBoRbacb7eSdbFT3Sp1-sR9dAOCkGXI";

int main() {
    
    // ============================================================================
    // 2. مسار الـ API المباشر لجلـب المناجم (/api/mines)
    // ============================================================================
    app().registerHandler(
        "/api/mines",
        [](const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
            
            // معالجة طلبات CORS Preflight (OPTIONS)
            if (req->method() == Options) {
                auto res = HttpResponse::newHttpResponse();
                res->setStatusCode(k200OK);
                res->addHeader("Access-Control-Allow-Origin", "*");
                res->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
                res->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, apikey");
                callback(res);
                return;
            }

            // قراءة الفلاتر من الطلب
            std::string mineral = req->getParameter("mineral");
            std::string company = req->getParameter("company");
            std::string province = req->getParameter("province");
            std::string minProd = req->getParameter("min_prod");

            auto client = HttpClient::newHttpClient(SUPABASE_URL);
            
            // بناء رابط الطلب الأساسي
            std::string path = "/rest/v1/mines?select=*";

            // إضافة الشروط فقط إذا لم تكن "all" أو غير فارغة
            if (!mineral.empty() && mineral != "all" && mineral != "undefined") {
                path += "&primary_mineral=eq." + mineral;
            }
            if (!company.empty() && company != "all" && company != "undefined") {
                path += "&operator=eq." + company;
            }
            if (!province.empty() && province != "all" && province != "undefined") {
                path += "&province=eq." + province;
            }
            if (!minProd.empty() && minProd != "0" && minProd != "undefined") {
                path += "&monthly_production_tons=gte." + minProd;
            }

            auto supabaseReq = HttpRequest::newHttpRequest();
            supabaseReq->setPath(path);
            supabaseReq->setMethod(drogon::Get);

            // إضافة الهيدرز الرسمية لـ Supabase
            supabaseReq->addHeader("apikey", SUPABASE_KEY);
            supabaseReq->addHeader("Authorization", "Bearer " + SUPABASE_KEY);
            supabaseReq->addHeader("Accept", "application/json");

            // إرسال الطلب إلى Supabase
            client->sendRequest(supabaseReq, [callback](ReqResult result, const HttpResponsePtr &response) {
                auto res = HttpResponse::newHttpResponse();
                
                res->addHeader("Access-Control-Allow-Origin", "*");
                res->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
                res->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, apikey");

                if (result == ReqResult::Ok && response && (response->getStatusCode() == k200OK || response->getStatusCode() == k206PartialContent)) {
                    res->setStatusCode(k200OK);
                    res->setContentTypeCode(CT_APPLICATION_JSON);
                    res->setBody(std::string(response->getBody()));
                } else {
                    res->setStatusCode(k200OK);
                    res->setContentTypeCode(CT_APPLICATION_JSON);
                    res->setBody("[]");
                }
                callback(res);
            });
        },
        {Get, Options}
    );

    // ============================================================================
    // 3. إعداد المجلد الثابت والصفحة الرئيسية
    // ============================================================================
    app().setDocumentRoot("./public");
    app().setHomePage("index.html");

    // المنفذ الخاص بـ Render
    int port = 10000;
    if (const char* envPort = std::getenv("PORT")) {
        port = std::atoi(envPort);
    }

    std::cout << "Drogon C++ Engine running on port " << port << std::endl;
    
    app().addListener("0.0.0.0", port);
    app().run();

    return 0;
}
