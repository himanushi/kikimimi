#include "realtime_protocol.h"

#include <ArduinoJson.h>

namespace realtime_protocol {

std::string buildSessionUpdateEvent(const std::string& instructions) {
    JsonDocument doc;
    doc["type"] = "session.update";
    JsonObject session = doc["session"].to<JsonObject>();
    session["type"] = "realtime";
    session["model"] = "gpt-realtime-mini";
    JsonArray modalities = session["output_modalities"].to<JsonArray>();
    modalities.add("audio");
    session["instructions"] = instructions;

    JsonObject audio = session["audio"].to<JsonObject>();
    JsonObject input = audio["input"].to<JsonObject>();
    JsonObject inputFormat = input["format"].to<JsonObject>();
    inputFormat["type"] = "audio/pcm";
    inputFormat["rate"] = 24000;
    // 発話の切れ目はサーバ側 VAD に任せる(push-to-talk にしない。00003 plan)
    JsonObject turnDetection = input["turn_detection"].to<JsonObject>();
    turnDetection["type"] = "semantic_vad";
    // ユーザー発話の書き起こし(conversation.item.input_audio_transcription.completed を得るため)
    JsonObject transcription = input["transcription"].to<JsonObject>();
    transcription["model"] = "gpt-4o-mini-transcribe";

    JsonObject output = audio["output"].to<JsonObject>();
    JsonObject outputFormat = output["format"].to<JsonObject>();
    outputFormat["type"] = "audio/pcm";
    outputFormat["rate"] = 24000;  // 入力側と同様に必須(欠けると session.update がエラーになる)
    output["voice"] = "marin";

    std::string out;
    serializeJson(doc, out);
    return out;
}

std::string buildUserMessageEvent(const std::string& text) {
    JsonDocument doc;
    doc["type"] = "conversation.item.create";
    JsonObject item = doc["item"].to<JsonObject>();
    item["type"] = "message";
    item["role"] = "user";
    JsonArray content = item["content"].to<JsonArray>();
    JsonObject part = content.add<JsonObject>();
    part["type"] = "input_text";
    part["text"] = text;

    std::string out;
    serializeJson(doc, out);
    return out;
}

std::string buildResponseCreateEvent() {
    JsonDocument doc;
    doc["type"] = "response.create";

    std::string out;
    serializeJson(doc, out);
    return out;
}

std::string buildInputAudioAppendEvent(const std::string& base64Audio) {
    JsonDocument doc;
    doc["type"] = "input_audio_buffer.append";
    doc["audio"] = base64Audio;

    std::string out;
    serializeJson(doc, out);
    return out;
}

ServerEvent parseServerEvent(const std::string& json) {
    ServerEvent ev;
    JsonDocument doc;
    if (deserializeJson(doc, json)) return ev;  // 壊れた JSON は Unknown のまま

    const char* type = doc["type"] | "";
    if (std::string(type) == "session.created") {
        ev.type = ServerEventType::SessionCreated;
    } else if (std::string(type) == "session.updated") {
        ev.type = ServerEventType::SessionUpdated;
    } else if (std::string(type) == "response.output_text.delta") {
        ev.type = ServerEventType::ResponseOutputTextDelta;
        ev.textDelta = std::string(doc["delta"] | "");
    } else if (std::string(type) == "response.output_audio.delta") {
        ev.type = ServerEventType::ResponseOutputAudioDelta;
        ev.audioDelta = std::string(doc["delta"] | "");
    } else if (std::string(type) == "response.output_audio_transcript.delta") {
        ev.type = ServerEventType::OutputAudioTranscriptDelta;
        ev.transcriptDelta = std::string(doc["delta"] | "");
    } else if (std::string(type) == "conversation.item.input_audio_transcription.completed") {
        ev.type = ServerEventType::InputTranscriptionCompleted;
        ev.transcriptText = std::string(doc["transcript"] | "");
    } else if (std::string(type) == "input_audio_buffer.speech_started") {
        ev.type = ServerEventType::SpeechStarted;
    } else if (std::string(type) == "input_audio_buffer.speech_stopped") {
        ev.type = ServerEventType::SpeechStopped;
    } else if (std::string(type) == "response.done") {
        ev.type = ServerEventType::ResponseDone;
        JsonObjectConst usage = doc["response"]["usage"];
        JsonObjectConst inputDetails = usage["input_token_details"];
        JsonObjectConst cachedDetails = inputDetails["cached_tokens_details"];
        JsonObjectConst outputDetails = usage["output_token_details"];
        ev.usage.inputTextTokens = inputDetails["text_tokens"] | 0L;
        ev.usage.inputAudioTokens = inputDetails["audio_tokens"] | 0L;
        ev.usage.cachedTextTokens = cachedDetails["text_tokens"] | 0L;
        ev.usage.cachedAudioTokens = cachedDetails["audio_tokens"] | 0L;
        ev.usage.outputTextTokens = outputDetails["text_tokens"] | 0L;
        ev.usage.outputAudioTokens = outputDetails["audio_tokens"] | 0L;
    } else if (std::string(type) == "error") {
        ev.type = ServerEventType::Error;
        ev.errorMessage = std::string(doc["error"]["message"] | "");
    }
    return ev;
}

}  // namespace realtime_protocol
