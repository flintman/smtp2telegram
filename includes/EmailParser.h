// EmailParser.h
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Email parsing and MIME decoding

#ifndef EMAIL_PARSER_H
#define EMAIL_PARSER_H

#include <string>
#include <map>

struct ParsedEmail {
    std::string subject;
    std::string from;
    std::string to;
    std::string body;
    std::string content_type;
    std::map<std::string, std::string> headers;
};

class EmailParser {
public:
    EmailParser();

    // Parse raw email data
    ParsedEmail parse(const std::string& raw_data);

    // Format parsed email for Telegram
    std::string formatForTelegram(const ParsedEmail& email);

private:
    std::string decodeQuotedPrintable(const std::string& input);
    std::string decodeBase64(const std::string& input);
    std::string stripHtmlTags(const std::string& html);
    std::string decodeHeader(const std::string& header);
    void parseHeaders(const std::string& header_section, ParsedEmail& email);
    std::string extractBoundary(const std::string& content_type);
    std::string parseMultipart(const std::string& body, const std::string& boundary);
};

#endif // EMAIL_PARSER_H
