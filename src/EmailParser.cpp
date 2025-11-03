// EmailParser.cpp
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Email parsing implementation

#include "../includes/EmailParser.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>

EmailParser::EmailParser() {
}

std::string EmailParser::decodeQuotedPrintable(const std::string& input) {
    std::string result;
    for (size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '=' && i + 2 < input.length()) {
            // Check if next two characters are hex digits
            if (std::isxdigit(input[i+1]) && std::isxdigit(input[i+2])) {
                std::string hex = input.substr(i + 1, 2);
                try {
                    result += static_cast<char>(std::stoi(hex, nullptr, 16));
                    i += 2;
                } catch (...) {
                    result += input[i];
                }
            } else if (input[i+1] == '\r' || input[i+1] == '\n') {
                // Soft line break - skip the = and the newline
                i++;
                if (input[i] == '\r' && i + 1 < input.length() && input[i+1] == '\n') {
                    i++;
                }
            } else {
                result += input[i];
            }
        } else {
            result += input[i];
        }
    }
    return result;
}

std::string EmailParser::decodeBase64(const std::string& input) {
    // Basic base64 decoding (simplified)
    const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    std::vector<int> vec(input.size());
    int val = 0, bits = -8;

    for (unsigned char c : input) {
        if (c == '=') break;
        size_t pos = base64_chars.find(c);
        if (pos == std::string::npos) continue;

        val = (val << 6) + pos;
        bits += 6;

        if (bits >= 0) {
            result.push_back(char((val >> bits) & 0xFF));
            bits -= 8;
        }
    }

    return result;
}

std::string EmailParser::stripHtmlTags(const std::string& html) {
    std::string result;
    bool in_tag = false;

    for (char c : html) {
        if (c == '<') {
            in_tag = true;
        } else if (c == '>') {
            in_tag = false;
        } else if (!in_tag) {
            result += c;
        }
    }

    return result;
}

std::string EmailParser::decodeHeader(const std::string& header) {
    // Simple implementation - can be enhanced for RFC 2047 encoded words
    return header;
}

void EmailParser::parseHeaders(const std::string& header_section, ParsedEmail& email) {
    std::istringstream stream(header_section);
    std::string line;
    std::string current_header;
    std::string current_value;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) break; // End of headers

        // Check if line is a continuation (starts with whitespace)
        if (!line.empty() && (line[0] == ' ' || line[0] == '\t')) {
            current_value += " " + line.substr(1);
            continue;
        }

        // Save previous header if exists
        if (!current_header.empty()) {
            email.headers[current_header] = current_value;

            if (current_header == "Subject") {
                email.subject = decodeHeader(current_value);
            } else if (current_header == "From") {
                email.from = current_value;
            } else if (current_header == "To") {
                email.to = current_value;
            } else if (current_header == "Content-Type") {
                email.content_type = current_value;
            }
        }

        // Parse new header
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            current_header = line.substr(0, colon);
            current_value = line.substr(colon + 1);

            // Trim leading whitespace from value
            size_t start = current_value.find_first_not_of(" \t");
            if (start != std::string::npos) {
                current_value = current_value.substr(start);
            }
        }
    }

    // Save last header
    if (!current_header.empty()) {
        email.headers[current_header] = current_value;

        if (current_header == "Subject") {
            email.subject = decodeHeader(current_value);
        } else if (current_header == "From") {
            email.from = current_value;
        } else if (current_header == "To") {
            email.to = current_value;
        } else if (current_header == "Content-Type") {
            email.content_type = current_value;
        }
    }
}

std::string EmailParser::extractBoundary(const std::string& content_type) {
    size_t boundary_pos = content_type.find("boundary=");
    if (boundary_pos == std::string::npos) return "";

    std::string boundary = content_type.substr(boundary_pos + 9);

    // Remove quotes if present
    if (!boundary.empty() && boundary[0] == '"') {
        boundary = boundary.substr(1);
        size_t end_quote = boundary.find('"');
        if (end_quote != std::string::npos) {
            boundary = boundary.substr(0, end_quote);
        }
    } else {
        // No quotes, take until semicolon or end
        size_t end = boundary.find(';');
        if (end != std::string::npos) {
            boundary = boundary.substr(0, end);
        }
    }

    return boundary;
}

std::string EmailParser::parseMultipart(const std::string& body, const std::string& boundary) {
    if (boundary.empty()) return body;

    std::string delimiter = "--" + boundary;
    size_t pos = body.find(delimiter);
    std::string text_content;

    while (pos != std::string::npos) {
        size_t start = pos + delimiter.length();
        size_t end = body.find(delimiter, start);

        if (end == std::string::npos) break;

        std::string part = body.substr(start, end - start);

        // Check if this is a text part
        if (part.find("Content-Type: text/plain") != std::string::npos) {
            // Find the body of this part (after double newline)
            size_t body_start = part.find("\n\n");
            if (body_start == std::string::npos) {
                body_start = part.find("\r\n\r\n");
                if (body_start != std::string::npos) body_start += 4;
            } else {
                body_start += 2;
            }

            if (body_start != std::string::npos) {
                text_content = part.substr(body_start);
                break; // Found text/plain part
            }
        }

        pos = end;
    }

    return text_content.empty() ? body : text_content;
}

ParsedEmail EmailParser::parse(const std::string& raw_data) {
    ParsedEmail email;

    // Split headers and body
    size_t header_end = raw_data.find("\n\n");
    if (header_end == std::string::npos) {
        header_end = raw_data.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            header_end += 4;
        }
    } else {
        header_end += 2;
    }

    if (header_end != std::string::npos) {
        std::string headers = raw_data.substr(0, header_end);
        std::string body = raw_data.substr(header_end);

        parseHeaders(headers, email);

        // Check if multipart
        std::string boundary = extractBoundary(email.content_type);
        if (!boundary.empty()) {
            body = parseMultipart(body, boundary);
        }

        // Check for quoted-printable encoding
        if (email.content_type.find("quoted-printable") != std::string::npos) {
            body = decodeQuotedPrintable(body);
        }

        // Check for HTML content
        if (email.content_type.find("text/html") != std::string::npos) {
            body = stripHtmlTags(body);
        }

        email.body = body;
    } else {
        // No clear header/body separation, treat all as body
        email.body = raw_data;
    }

    return email;
}

std::string EmailParser::formatForTelegram(const ParsedEmail& email) {
    std::ostringstream oss;

    if (!email.from.empty()) {
        oss << "From: " << email.from << "\n";
    }

    if (!email.subject.empty()) {
        oss << "Subject: " << email.subject << "\n";
    }

    if (!email.from.empty() || !email.subject.empty()) {
        oss << "\n";
    }

    oss << email.body;

    return oss.str();
}
