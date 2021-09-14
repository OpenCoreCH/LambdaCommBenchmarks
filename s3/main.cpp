#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/lambda-runtime/runtime.h>
#include <vector>
#include <random>
#include <chrono>
#include <cstdint>
#include <iostream>


using namespace aws::lambda_runtime;
using random_bytes_engine = std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char>;
char const TAG[] = "LAMBDA_ALLOC";

void upload_random_file(Aws::S3::S3Client const &client,
                        Aws::String const &bucket,
                        Aws::String const &key,
                        int size);


static invocation_response my_handler(invocation_request const &req, Aws::S3::S3Client const &client)
{
    std::cout << "Invoked handler" << std::endl;
    using namespace Aws::Utils::Json;
    JsonValue json(req.payload);
    if (!json.WasParseSuccessful())
    {
        return invocation_response::failure("Failed to parse input JSON", "InvalidJSON");
    }

    auto v = json.View();

    auto bucket = v.GetString("s3bucket");
    auto key = v.GetString("s3key");
    auto role = v.GetString("role"); // producer or consumer
    auto file_size = v.GetInteger("fileSize");

    if (role == "producer")
    {
        upload_random_file(client, bucket, key, file_size);
    }
    else if (role == "consumer")
    {
    }
    return invocation_response::success("finished", "text/plain");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory()
{
    return []
    {
        return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
            "console_logger", Aws::Utils::Logging::LogLevel::Trace);
    };
}

int main()
{
    using namespace Aws;
    SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
    options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
    InitAPI(options);
    {
        Client::ClientConfiguration config;
        config.region = Aws::Environment::GetEnv("AWS_REGION");
        config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";

        auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
        S3::S3Client client(credentialsProvider, config);
        auto handler_fn = [&client](aws::lambda_runtime::invocation_request const &req)
        {
            return my_handler(req, client);
        };
        run_handler(handler_fn);
    }
    ShutdownAPI(options);
    return 0;
}

void upload_random_file(Aws::S3::S3Client const &client,
                        Aws::String const &bucket,
                        Aws::String const &key,
                        int size)
{
    using namespace Aws;
    random_bytes_engine rbe;
    std::vector<char> data(size);
    std::generate(begin(data), end(data), std::ref(rbe));
    std::cout << "Generated random data" << std::endl;

    S3::Model::PutObjectRequest request;
    request.WithBucket(bucket).WithKey(key);
    std::string s(data.begin(), data.end());
    const std::shared_ptr<Aws::IOStream> input_data =
        Aws::MakeShared<Aws::StringStream>("");
    *input_data << s.c_str();
    request.SetBody(input_data);
    S3::Model::PutObjectOutcome outcome = client.PutObject(request);
}