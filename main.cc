#include <drogon/drogon.h>
#include <json/json.h>
#include <iostream>
#include <cstdlib>
#include <algorithm>

using namespace drogon;

// بيانات الاتصال بـ Supabase
const std::string SUPABASE_URL = "https://pxwlzbxfnzbijazwtfti.supabase.co";
const std::string SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InB4d2x6YnhmbnpiaWphend0ZnRpIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODU0NDA2MjUsImV4cCI6MjEwMTAxNjYyNX0.KAWrEx5koLOVrBoRbacb7eSdbFT3Sp1-sR9dAOCkGXI";

// دالة مساعدة للتحقق مما إذا كانت القيمة ملغاة أو "all"
bool isValidFilter(const std::string& val) {
    if (val.empty() || val == "all" || val == "undefined" || val == "null" || val == "0") {
        return false;
    }
    return true;
}

int main() {
    
    // تسجيل API /api/mines
    app().registerHandler(
        "/api/mines",
        [](const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback) {
            
            // معالجة طلبات CORS Preflight
            if (req->method() == Options) {
                auto res = HttpResponse::newHttpResponse();
                res->setStatusCode(k200OK);
                res->addHeader("Access-Control-Allow-Origin", "*");
                res->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
                res->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, apikey");
                callback(res);
                return;
            }

            // قراءة المتغيرات من رابط الطلب
            std::string mineral = req->getParameter("mineral");
            std::string company = req->getParameter("company");
            std::string province = req->getParameter("province");
            std::string minProd = req->getParameter("min_prod");

            auto client = HttpClient::newHttpClient(SUPABASE_URL);
            
            // الرابط الأساسي لجلب جميع البيانات
            std::string path = "/rest/v1/mines?select=*";

            // إرفاق الفلاتر فقط إذا كانت تحتوي على قيمة حقيقية وليست "all"
            if (isValidFilter(mineral)) {
                path += "&primary_mineral=ilike.*" + mineral + "*";
            }
            if (isValidFilter(company)) {
                path += "&operator=ilike.*" + company + "*";
            }
            if (isValidFilter(province)) {
                path += "&province=ilike.*" + province + "*";
            }
            if (isValidFilter(minProd)) {
                path += "&monthly_production_tons=gte." + minProd;
            }

            // طباعة المسار في السجل للتأكد من صحة الرابط المرسل لـ Supabase
            std::cout << "[Supabase Request]: " << path << std::endl;

            auto supabaseReq = HttpRequest::newHttpRequest();
            supabaseReq->setPath(path);
            supabaseReq->setMethod(drogon::Get);

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

    // إعداد المجلد الثابت
    app().setDocumentRoot("./public");
    app().setHomePage("index.html");

    int port = 10000;
    if (const char* envPort = std::getenv("PORT")) {
        port = std::atoi(envPort);
    }

    std::cout << "Drogon C++ Engine active on port " << port << std::endl;
    app().addListener("0.0.0.0", port);
    app().run();

    return 0;
}
