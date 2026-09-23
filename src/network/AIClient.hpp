#pragma once

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

class AIClient {
  public:
    enum class Status { Idle, Loading, Success, Error };

    AIClient() = default;
    ~AIClient() =
        default; // std::jthread se unirá automáticamente al destruirse

    // Despacha la petición en segundo plano
    void RequestLayoutAsync(const std::string& endpointUrl,
                            const std::string& apiKey,
                            const nlohmann::json& payload);

    // Métodos para consultar desde el hilo principal (Render / ImGui)
    [[nodiscard]] Status GetStatus() const noexcept { return m_Status.load(); }
    [[nodiscard]] bool IsLoading() const noexcept {
        return m_Status.load() == Status::Loading;
    }

    // Consume la respuesta (devuelve el contenido solo una vez tras el éxito)
    std::optional<std::string> PollResult();
    // Consume el último error una sola vez y vuelve al estado Idle.
    std::optional<std::string> PollError();

  private:
    std::atomic<Status> m_Status{Status::Idle};
    mutable std::mutex m_Mutex;
    std::string m_ResponseContent;
    std::string m_ErrorMessage;

    std::jthread m_WorkerThread;
};
