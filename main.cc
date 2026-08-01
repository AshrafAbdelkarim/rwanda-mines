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
    // 2. مسار الـ API المصلح بالكامل (يلتقط /api/mines وكل معلمات التصفية)
    // ============================================================================
    app().registerHandler("^/api/mines.*$", [](const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
        
        // معالجة طلبات Preflight الخاصة بـ CORS (OPTIONS)
        if (req->method() == Options) {
            auto res = HttpResponse::newHttpResponse();
            res->setStatusCode(k200OK);
            res->addHeader("Access-Control-Allow-Origin", "*");
            res->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            res->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, apikey");
            callback(res);
            return;
        }

        // قراءة معلمات التصفية
        auto mineral = req->getParameter("mineral");
        auto company = req->getParameter("company");
        auto province = req->getParameter("province");
        auto minProd = req->getParameter("min_prod");

        auto client = HttpClient::newHttpClient(SUPABASE_URL);
        
        // بناء الاستعلام لـ Supabase REST API
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

        // الهيدرز الخاصة بـ Supabase
        supabaseReq->addHeader("apikey", SUPABASE_KEY);
        supabaseReq->addHeader("Authorization", "Bearer " + SUPABASE_KEY);
        supabaseReq->addHeader("Accept", "application/json");

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
    }, {Get, Options});

    // ============================================================================
    // 3. إتاحة الصفحات والملفات الثابتة
    // ============================================================================
    app().registerHandler("/", [](const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
        auto fileResp = HttpResponse::newFileResponse("./public/index.html");
        callback(fileResp);
    }, {Get});

    app().setDocumentRoot("./public");

    // المنفذ الخاص بـ Render
    int port = 10000;
    if (const char* envPort = std::getenv("PORT")) {
        port = std::atoi(envPort);
    }

    std::cout << "Running Drogon C++ Server on Render, Port: " << port << std::endl;
    
    app().addListener("0.0.0.0", port);
    app().run();

    return 0;
}
