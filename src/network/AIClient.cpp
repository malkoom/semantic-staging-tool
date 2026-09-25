#include "AIClient.hpp"

namespace {
constexpr std::size_t kMaxErrorBodyLength = 512;

std::string FormatHttpError(const cpr::Response& response) {
    std::string message = "HTTP " + std::to_string(response.status_code);

    try {
        const auto body = nlohmann::json::parse(response.text);
        if (body.contains("error") && body["error"].contains("message") &&
            body["error"]["message"].is_string()) {
            return message + ": " + body["error"]["message"].get<std::string>();
        }
    } catch (const nlohmann::json::exception&) {
        // Si el servidor no devuelve JSON, usamos una versión acotada del body.
    }

    if (!response.text.empty()) {
        message += ": " + response.text.substr(0, kMaxErrorBodyLength);
        if (response.text.size() > kMaxErrorBodyLength) {
            message += "...";
        }
    }
    return message;
}
} // namespace

void AIClient::RequestLayoutAsync(const std::string& endpointUrl,
                                  const std::string& apiKey,
                                  const nlohmann::json& payload) {
    // Si ya hay un hilo corriendo, reasignarlo invoca su destructor (jthread se
    // une)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_ResponseContent.clear();
        m_ErrorMessage.clear();
    }
    m_Status.store(Status::Loading);

    m_WorkerThread =
        std::jthread([this, endpointUrl, apiKey,
                      payloadJson = payload.dump()](std::stop_token stopToken) {
            // Ejecución HTTP bloqueante (pero confinada al worker thread)
            cpr::Response response =
                cpr::Post(cpr::Url{endpointUrl},
                          cpr::Header{{"Content-Type", "application/json"},
                                      {"Authorization", "Bearer " + apiKey}},
                          cpr::Body{payloadJson},
                          cpr::Timeout{15000} // 15 segundos de timeout
                );

            // Si el hilo principal solicitó detenerse, descartamos el
            // procesamiento
            if (stopToken.stop_requested()) {
                return;
            }

            std::lock_guard<std::mutex> lock(m_Mutex);

            if (response.status_code == 200) {
                try {
                    // Parseamos la respuesta externa de OpenAI/Groq para
                    // extraer solo el texto del asistente
                    auto jsonResponse = nlohmann::json::parse(response.text);
                    m_ResponseContent =
                        jsonResponse["choices"][0]["message"]["content"]
                            .get<std::string>();

                    m_Status.store(Status::Success);
                } catch (const std::exception& e) {
                    m_ErrorMessage =
                        std::string("Fallo al parsear respuesta JSON: ") +
                        e.what();
                    m_Status.store(Status::Error);
                }
            } else if (response.status_code == 0) {
                m_ErrorMessage =
                    "Error de red o timeout: " + response.error.message;
                m_Status.store(Status::Error);
            } else {
                m_ErrorMessage = FormatHttpError(response);
                m_Status.store(Status::Error);
            }
        });
}

std::optional<std::string> AIClient::PollResult() {
    if (m_Status.load() != Status::Success) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Status.store(Status::Idle); // Reset a idle tras consumir
    return std::move(m_ResponseContent);
}

std::optional<std::string> AIClient::PollError() {
    if (m_Status.load() != Status::Error) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Status.store(Status::Idle);
    return std::move(m_ErrorMessage);
}
