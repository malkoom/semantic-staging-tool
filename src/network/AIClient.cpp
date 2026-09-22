#include "AIClient.hpp"

void AIClient::RequestLayoutAsync(const std::string& endpointUrl,
                                  const std::string& apiKey,
                                  const nlohmann::json& payload) {
    // Si ya hay un hilo corriendo, reasignarlo invoca su destructor (jthread se
    // une)
    m_Status.store(Status::Loading);

    m_WorkerThread = std::jthread([this, endpointUrl, apiKey,
                                   payloadJson = payload.dump()](
                                      std::stop_token stopToken) {
        // Ejecución HTTP bloqueante (pero confinada al worker thread)
        cpr::Response response =
            cpr::Post(cpr::Url{endpointUrl},
                      cpr::Header{{"Content-Type", "application/json"},
                                  {"Authorization", "Bearer " + apiKey}},
                      cpr::Body{payloadJson},
                      cpr::Timeout{15000} // 15 segundos de timeout
            );

        // Si el hilo principal solicitó detenerse, descartamos el procesamiento
        if (stopToken.stop_requested()) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_Mutex);

        if (response.status_code == 200) {
            try {
                // Parseamos la respuesta externa de OpenAI/Groq para extraer
                // solo el texto del asistente
                auto jsonResponse = nlohmann::json::parse(response.text);
                m_ResponseContent =
                    jsonResponse["choices"][0]["message"]["content"]
                        .get<std::string>();
                m_Status.store(Status::Success);
            } catch (const std::exception& e) {
                m_ErrorMessage =
                    std::string("Fallo al parsear respuesta JSON: ") + e.what();
                m_Status.store(Status::Error);
            }
        } else {
            m_ErrorMessage = "HTTP " + std::to_string(response.status_code) +
                             ": " + response.error.message;
            if (!response.text.empty()) {
                m_ErrorMessage += " | " + response.text;
            }
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

std::string AIClient::GetLastError() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ErrorMessage;
}
