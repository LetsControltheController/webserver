/* HTTP File Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_event_loop.h"

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_err.h"
#include "esp_vfs.h"
#include "esp_http_server.h"

#define CONFIG_AP_SSID_HIDDEN 1
#define MAX_APs 20
#define STA_SSID "test_for_esp"
#define STA_PASSWORD "12341234"
#define AP_SSID "ESPTEST"
#define AP_PASSWORD "12345678"
#define AP_MAX_CONN 4
#define AP_CHANNEL 0
/* Max length a file path can have on storage */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/* Max size of an individual file. Make sure this

#define MAX_FILE_SIZE (200 * 1024) // 200 KB
#define MAX_FILE_SIZE_STR "200KB" */

/* Scratch buffer size */
#define SCRATCH_BUFSIZE 8192
/* This example demonstrates how to create file server
 * using esp_http_server. This file has only startup code.
 * Look in file_server.c for the implementation */
char parsed[5][25];
char parse[100];
char size_ssid[35];
char ssid_get[30] = "ssid", pass_get[25] = "pass", conn_get[10] = "wifi";
struct file_server_data
{
    /* Base path of file storage */
    char base_path[ESP_VFS_PATH_MAX + 1];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};

static const char *TAG = "file_server";

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}
/* Function to initialize SPIFFS */
static esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5, // This decides the maximum number of files that can be created on the storage
        .format_if_mount_failed = true};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    return ESP_OK;
}

uint16_t ap_num = MAX_APs;
wifi_ap_record_t ap_records[MAX_APs];
void scann()
{

    // configure and run the scan process in blocking mode
    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = false};

    printf("Wifi scanning...");
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));

    ESP_ERROR_CHECK(esp_wifi_start());
}
static esp_err_t http_resp_dir_html(httpd_req_t *req, const char *dirpath);
static esp_err_t index_html_get_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/index_html");
    httpd_resp_send(req, NULL, 0); // Response body can be empty
    return ESP_OK;
}

static esp_err_t logo_get_handler(httpd_req_t *req)
{
    extern const unsigned char logo_png_start[] asm("_binary_logo_png_start");
    extern const unsigned char logo_png_end[] asm("_binary_logo_png_end");
    const size_t logo_png_size = (logo_png_end - logo_png_start);
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)logo_png_start, logo_png_size);
    return ESP_OK;
}
static esp_err_t youtube_get_handler(httpd_req_t *req)
{
    extern const unsigned char youtube_png_start[] asm("_binary_youtube_png_start");
    extern const unsigned char youtube_png_end[] asm("_binary_youtube_png_end");
    const size_t youtube_png_size = (youtube_png_end - youtube_png_start);
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)youtube_png_start, youtube_png_size);
    return ESP_OK;
}
static esp_err_t logo2_get_handler(httpd_req_t *req)
{
    extern const unsigned char logo2_png_start[] asm("_binary_logo2_png_start");
    extern const unsigned char logo2_png_end[] asm("_binary_logo2_png_end");
    const size_t logo2_png_size = (logo2_png_end - logo2_png_start);
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)logo2_png_start, logo2_png_size);
    return ESP_OK;
}
static esp_err_t instagram_get_handler(httpd_req_t *req)
{
    extern const unsigned char instagram_png_start[] asm("_binary_instagram_png_start");
    extern const unsigned char instagram_png_end[] asm("_binary_instagram_png_end");
    const size_t instagram_png_size = (instagram_png_end - instagram_png_start);
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)instagram_png_start, instagram_png_size);
    return ESP_OK;
}
static esp_err_t linkedin_get_handler(httpd_req_t *req)
{
    extern const unsigned char linkedin_png_start[] asm("_binary_linkedin_png_start");
    extern const unsigned char linkedin_png_end[] asm("_binary_linkedin_png_end");
    const size_t linkedin_png_size = (linkedin_png_end - linkedin_png_start);
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)linkedin_png_start, linkedin_png_size);
    return ESP_OK;
}
static esp_err_t location_get_handler(httpd_req_t *req)
{
    extern const unsigned char location_png_start[] asm("_binary_location_png_start");
    extern const unsigned char location_png_end[] asm("_binary_location_png_end");
    const size_t location_png_size = (location_png_end - location_png_start);
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)location_png_start, location_png_size);
    return ESP_OK;
}
static esp_err_t facebook_get_handler(httpd_req_t *req)
{
    extern const unsigned char facebook_png_start[] asm("_binary_facebook_png_start");
    extern const unsigned char facebook_png_end[] asm("_binary_facebook_png_end");
    const size_t facebook_png_size = (facebook_png_end - facebook_png_start);
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)facebook_png_start, facebook_png_size);
    return ESP_OK;
}
static esp_err_t mail_get_handler(httpd_req_t *req)
{
    extern const unsigned char mail_png_start[] asm("_binary_mail_png_start");
    extern const unsigned char mail_png_end[] asm("_binary_mail_png_end");
    const size_t mail_png_size = (mail_png_end - mail_png_start);
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)mail_png_start, mail_png_size);
    return ESP_OK;
}
static esp_err_t mobile_get_handler(httpd_req_t *req)
{
    extern const unsigned char mobile_png_start[] asm("_binary_mobile_png_start");
    extern const unsigned char mobile_png_end[] asm("_binary_mobile_png_end");
    const size_t mobile_png_size = (mobile_png_end - mobile_png_start);
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)mobile_png_start, mobile_png_size);
    return ESP_OK;
}
static esp_err_t web_get_handler(httpd_req_t *req)
{
    extern const unsigned char web_png_start[] asm("_binary_web_png_start");
    extern const unsigned char web_png_end[] asm("_binary_web_png_end");
    const size_t web_png_size = (web_png_end - web_png_start);
    httpd_resp_set_type(req, "image/png");
    httpd_resp_send(req, (const char *)web_png_start, web_png_size);
    return ESP_OK;
}

static esp_err_t connect_get_handler(httpd_req_t *req)
{
    char *token = strtok(parse, "?");
    token = strtok(NULL, "?");

    printf("after(?) token:%s \n", token);
    if (!(strncmp("home", token, 4)))
    {
        httpd_resp_set_status(req, "307 Temporary Redirect");
        httpd_resp_set_hdr(req, "Location", "/index_html");
        httpd_resp_send(req, NULL, 0);
    }
    if (!(strncmp("cont", token, 4)))
    {
        httpd_resp_set_status(req, "307 Temporary Redirect");
        httpd_resp_set_hdr(req, "Location", "/contact");
        httpd_resp_send(req, NULL, 0);
    }
    if (!(strncmp("abou", token, 4)))
    {
        httpd_resp_set_status(req, "307 Temporary Redirect");
        httpd_resp_set_hdr(req, "Location", "/about");
        httpd_resp_send(req, NULL, 0);
    }
    if (!(strncmp("ssid", token, 4)))
    {

        strcpy(ssid_get, " ");
        strcpy(pass_get, " ");
        token = strtok(token, "&");
        printf(" %s \t", token);
        stpcpy(parsed[0], token + 5);
        strcpy(ssid_get, parsed[0]);
        printf("ssid detected! %s     \n", ssid_get);
        token = strtok(NULL, "&");
        printf("%s \t", token);
        stpcpy(parsed[1], token + 4);
        strcpy(pass_get, parsed[1]);
        printf("pass detected! %s     \n", pass_get);
    }
    else if (!(strncmp("conn", token, 4)))
    {
        strcpy(conn_get, "");
        token = strtok(token, "&");
        printf(" %s \t", token);
        stpcpy(parsed[0], token + 12);
        strcpy(conn_get, parsed[0]);
        printf("connection type detected! %s     \n", conn_get);
    }
    else
    {
        printf("could not find any data! \n");
        //	 httpd_resp_send_chunk(req,  " alert('error!')/*displays error message*/",-1);
        httpd_resp_set_status(req, "307 Temporary Redirect");
        httpd_resp_set_hdr(req, "Location", "/index_html");
        httpd_resp_send(req, NULL, 0); // Response body can be empty
    }
    //httpd_resp_send_chunk(req,  " SAVED!",-1);
    // httpd_resp_send_chunk(req,  " alert('changed values are saved!')/*displays error message*/",-1);
    //    printf ("new connection type : %s   %s \n",parsed[0] ,parsed[1]);
    printf("going to success\n");
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/index_html");
    httpd_resp_send(req, NULL, 0); // Response body can be empty
    return ESP_OK;
}

static esp_err_t contact_resp_dir_html(httpd_req_t *req, const char *dirpath)
{
    httpd_resp_send_chunk(req,
                          "<!DOCTYPE html>"
                          "<html>"
                          "<head>"
                          "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /> "
                          "<style>"
                          "body {margin:0;}"

                          "ul {"
                          "  list-style-type: none;"
                          "  margin: 0;"
                          "  padding: 0;"
                          "  overflow: hidden;"
                          "  background-color: #333;"
                          "  position: fixed;"
                          "  top: 0;"
                          "  width: 100%;"
                          "}"
                          "li {"
                          "  float: left;"

                          "}"

                          "li:last-child {"
                          "  border-right: none;"
                          "}"

                          "li a {"
                          "  display: block;"
                          "  color: white;"
                          "  text-align: center;"
                          "  padding: 14px 16px;"
                          "  text-decoration: none;"
                          "}"

                          "li a:hover:not(.active) {"
                          "  background-color: #111;"
                          "}"

                          ".active {"
                          "  background-color: #e00a0a;"
                          "}"
                          "</style>"
                          "</head>"
                          "<head>"
                          "<style>"
                          "* {"
                          "  box-sizing: border-box;"
                          "}"

                          "input[type=text], select, textarea {"
                          "  width: 100%;"
                          "  padding: 12px;"
                          "  border: 1px solid #ccc;"
                          "  border-radius: 4px;"
                          "  resize: vertical;"
                          "}"

                          "label {"
                          "  padding: 12px 12px 12px 0;"
                          "  display: inline-block;"
                          "}"

                          "input[type=submit] {"
                          "  background-color: #e00a0a;"
                          "  color: white;"
                          "  padding: 12px 20px;"
                          "  border: none;"
                          "  border-radius: 4px;"
                          "  cursor: pointer;"
                          "  float: right;"
                          "}"

                          "input[type=submit]:hover {"
                          "  background-color: #380a0a;"
                          "}"

                          ".container {"
                          "  border-radius: 5px;"
                          "  background-color: #f2f2f2;"
                          "  padding: 20px;"
                          "}"

                          ".col-25 {"
                          "  float: left;"
                          "  width: 25%;"
                          "  margin-top: 6px;"
                          "}"

                          ".col-75 {"
                          "  float: left;"
                          "  width: 75%;"
                          "  margin-top: 6px;"
                          "}"
                          ".col-55 {"
                          "  float: left;"
                          "  width: 55%;"
                          "  margin-top: 6px;"
                          "}"
                          "/* Clear floats after the columns */"
                          ".row:after {"
                          "  content: \"\";"
                          "  display: table;"
                          "  clear: both;"
                          "}"
                          /* Responsive layout - when the screen is less than 600px wide, make the two columns stack on top of each other instead of next to each other */
                          "@media screen and (max-width: 481px) {"
                          "  .col-25, .col-75, input[type=submit] {"
                          "    width: 100%;"
                          "    margin-top: 0;"
                          "  }"
                          "}"

                          "</style>"
                          "<style>"
                          "body {background-color: whitesmoke;}"
                          "b1   {color: #3659B8;}"

                          "h1   {color: blue;}"
                          "p    {color: black;}"
                          "</style>"
                          "</head>"

                          "<ul>"
                          "  <li><a href=\"?home\">Connections</a></li>"
                          "  <li><a class=\"active\"  href=\"?contact\">Contact</a></li>"
                          "  <li><a href=\"?about\">About</a></li> "
                          " <li style=\"float:right\"><a href=\"?about\"> <img src=\"logo2.png\" width=\"70\" height=\"14\" /></a></li>"
                          "</ul>"

                          "<body style=\"margin-top:70px;height:1500px;\">"
                          "<table border=\"0px\">"

                          "<tbody  style=\"padding:20px;margin-top:30px;background-color:#1abc9c;height:1500px;\">"

                          "  <p align='middle'><img src='web.png' alt='' width='18' height='18' /> "
                          "  <a title='https://www.company_name.com/' align='left' href='https://www.company_name.com/' target='_blank' rel='noopener' data-saferedirecturl='https://www.google.com/url?q=https://www.company_name.com/&amp;source=gmail&amp;ust=1547732724631000&amp;usg=AFQjCNHFlgAvZxHHJVtcGDzQgSFqupEjVA'>www.company_name.com</a> "
                          "    </p> "

                          "<p align='middle'><img src='mobile.png' alt='' width='11' height='17' /> "
                          "<a title='tel:+90000000000' href='tel:+900000000000' target='_blank' rel='noopener'>+90 (232) 000 00 00</a> "
                          "  </p> "

                          "<p align='middle'><img src='mail.png' alt='' width='17' height='17' /> "
                          "<a title='mailto:company_name@company_name.com' href='mailto:company_name@company_name.com' target='_blank' rel='noopener'>company_name@company_name.com</a> "
                          "  </p> "

                          "  <p align='middle'><img src='location.png' alt='' width='18' height='18' /> "
                          "  <strong> company_name </strong></p> "

                          "  <p align='middle'>Adress</p> "

                          " <p align='middle'> <a title='https://tr.linkedin.com/company/company_name' href='https://tr.linkedin.com/company/company_name' target='_blank' rel='noopener'><img  src=' linkedin.png' alt=' mobile.png' width='29' height='29' /></a> "
                          "<a title='https://www.facebook.com/company_name/' href='https://www.facebook.com/company_name/' target='_blank' rel='noopener' ><img  src=' facebook.png' alt='facebook' width='29' height='29' /></a> "
                          "<a title='https://www.youtube.com/company_name' href='https://www.youtube.com/company_name' target='_blank' rel='noopener' ><img  src=' youtube.png' alt=' facebook.png' width='29' height='29' /></a> "
                          "<a title='https://www.instagram.com/company_name_/' href='https://www.instagram.com/company_name_/' target='_blank' rel='noopener'><img src=' instagram.png' alt=' youtube.png' width='29' height='29' /></a> "
                          "</p> "
                          "</tbody>"
                          "</table>"
                          "</body>"
                          "</html>",
                          -1);

    return ESP_OK;
}

static esp_err_t about_resp_dir_html(httpd_req_t *req, const char *dirpath)
{
    httpd_resp_send_chunk(req,
                          " <!DOCTYPE html> "
                          " <html> "
                          " <head> "
                          " <meta name='viewport' content='width=device-width, initial-scale=1' />  "
                          " <style> "
                          " body {margin:0;} "
                          " ul { "
                          "   list-style-type: none; "
                          "   margin: 0; "
                          "   padding: 0; "
                          "   overflow: hidden; "
                          "   background-color: #333; "
                          "   position: fixed; "
                          "   top: 0; "
                          "   width: 100%; "
                          " } "
                          " li { "
                          "   float: left; "
                          " } "
                          " li:last-child { "
                          "   border-right: none; "
                          " } "
                          " li a { "
                          "   display: block; "
                          "   color: white; "
                          "   text-align: center; "
                          "   padding: 14px 16px; "
                          "   text-decoration: none; "
                          " } "
                          " li a:hover:not(.active) { "
                          "   background-color: #111; "
                          " } "
                          " .active { "
                          "   background-color: #e00a0a; "
                          " } "
                          " </style> "
                          " </head> "
                          " <head> "
                          " <style> "
                          " * { "
                          "   box-sizing: border-box; "
                          " } "
                          " input[type=text], select, textarea { "
                          "   width: 100%; "
                          "   padding: 12px; "
                          "   border: 1px solid #ccc; "
                          "   border-radius: 4px; "
                          "   resize: vertical; "
                          " } "
                          " label { "
                          "   padding: 12px 12px 12px 0; "
                          "   display: inline-block; "
                          " } "
                          " input[type=submit] { "
                          "   background-color: #e00a0a; "
                          "   color: white; "
                          "   padding: 12px 20px; "
                          "   border: none; "
                          "   border-radius: 4px; "
                          "   cursor: pointer; "
                          "   float: right; "
                          " } "
                          " input[type=submit]:hover { "
                          "   background-color: #380a0a; "
                          " } "
                          " .container { "
                          "   border-radius: 5px; "
                          "   background-color: #f2f2f2; "
                          "   padding: 20px; "
                          " } "
                          ""
                          " .col-25 { "
                          "   float: left; "
                          "   width: 25%; "
                          "   margin-top: 6px; "
                          " } "
                          " .col-75 { "
                          "   float: left; "
                          "   width: 75%; "
                          "   margin-top: 6px; "
                          " } "
                          " .col-55 { "
                          "   float: left; "
                          "   width: 55%; "
                          "   margin-top: 6px; "
                          " } "
                          " /* Clear floats after the columns */ "
                          " .row:after { "
                          "   content: ''; "
                          "   display: table; "
                          "   clear: both; "
                          " } "
                          "/* Responsive layout - when the screen is less than 600px wide, make the two columns stack on top of each other instead of next to each other */"
                          " @media screen and (max-width: 481px) { "
                          "   .col-25, .col-75, input[type=submit] { "
                          "     width: 100%; "
                          "     margin-top: 0; "
                          "   } "
                          " } "
                          " </style> "
                          " <style> "
                          " body {background-color: whitesmoke;} "
                          " b1   {color: #3659B8;} "
                          " h2   {color: red;} "
                          " h1   {color: blue;} "
                          " p    {color: black;} "
                          " </style> "
                          " </head> "
                          " <ul> "
                          "   <li><a href='?home'>Connections</a></li> "
                          "   <li><a href='?contact'>Contact</a></li> "
                          "   <li><a  class='active'  href='?about'>About</a></li>  "
                          " <li style=\"float:right\"><a href=\"?about\"> <img src=\"logo2.png\" width=\"70\" height=\"14\" /></a></li>"
                          " </ul> "
                          "	 <body align='middle' style='margin-top:70px;height:1500px;'>  "
                          "   <table border='0px'>  "
                          "  <p align='middle'><img src='logo.png' width='80' height='81' /></p> "
                          "   <tbody  style='padding:20px;margin-top:30px;height:1500px;'>  "
                          " <h2> Company Profile</h2>"
                          " <p>"
                          "company_name, which brings ideas into reality by offering R&D, design, production, application and after-sales services, is established and operates in technology development zone. With the vision of becoming the LEADER and global BRAND through sustainable, inspiring and simple transformative IoT technology, it continues to offer competitive products and solutions globally through its domestic work force. With over 10 years of experience, and with its domestic and foreign business partners, the company provides software platform solutions and electronic equipment production to many international firms in many countries including developed countries. Supported wireless technologies include GSM, GPRS, NB-IoT, Wi-Fi, Bluetooth Low Energy, NFC, LoRa and Sigfox and more.</p> "

                          " <p align='middle'> <a title='https://tr.linkedin.com/company/company_name' href='https://tr.linkedin.com/company/company_name' target='_blank' rel='noopener'><img  src=' linkedin.png' alt=' mobile.png' width='29' height='29' /></a> "
                          "<a title='https://www.facebook.com/company_name/' href='https://www.facebook.com/company_name/' target='_blank' rel='noopener' ><img  src=' facebook.png' alt='facebook' width='29' height='29' /></a> "
                          "<a title='https://www.youtube.com/company_name' href='https://www.youtube.com/company_name' target='_blank' rel='noopener' ><img  src=' youtube.png' alt=' facebook.png' width='29' height='29' /></a> "
                          "<a title='https://www.instagram.com/company_name_/' href='https://www.instagram.com/company_name_/' target='_blank' rel='noopener'><img src=' instagram.png' alt=' youtube.png' width='29' height='29' /></a> "
                          "</p> "
                          "  </tbody> "
                          "  </table> "
                          " </body>  </html> ",
                          -1);

    return ESP_OK;
}

static esp_err_t ask_resp_dir_html(httpd_req_t *req, const char *dirpath)
{

    httpd_resp_send_chunk(req,

                          "<tr>"
                          "<head>"
                          "<style>"
                          "body {background-color: whitesmoke;}"
                          "h1   {color: blue;}"
                          "p    {color: black;}"
                          "</style>"
                          "</head>"
                          "<body>"
                          "<p align=\"middle\"><img src=\"logo.png\" width=\"80\" height=\"81\" /></p>"

                          "</tr>"
                          "<tr>"
                          "<tr>"
                          "<p align=\"middle\"><img src=\"web.png\" alt=\"\" width=\"18\" height=\"18\" />"
                          "<a title=\"https://www.company_name.com/\" align=\"left\" href=\"https://www.company_name.com/\" target=\"_blank\" rel=\"noopener\" data-saferedirecturl=\"https://www.google.com/url?q=https://www.company_name.com/&amp;source=gmail&amp;ust=1547732724631000&amp;usg=AFQjCNHFlgAvZxHHJVtcGDzQgSFqupEjVA\">www.company_name.com</a>"
                          "</p>"
                          "</tr>"
                          "</tr>"
                          "<tr>"
                          "<p align=\"middle\"><img src=\"location.png\" alt=\"\" width=\"18\" height=\"18\" />"
                          "<strong>company_name</strong></p>"
                          "</tr>"
                          "<tr>"
                          "<p align=\"middle\">Address<br />Country</p>"

                          "<form name='loginForm'>"
                          "<table width='20%' bgcolor='A09F9F' align='center'>"
                          "<tr>"
                          "<td colspan=2>"
                          "<center><font size=4><b>Gateway Login Page</b></font></center>"

                          "</td>"

                          "</tr>"
                          "<td>Username:</td>"
                          "<td><input type='text' size=25 name='userid'><br></td>"
                          "</tr>"

                          "<tr>"
                          "<td>Password:</td>"
                          "<td><input type='Password' size=25 name='pwd'><br></td>"
                          "<br>"
                          "<br>"
                          "</tr>"
                          "<tr>"
                          "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
                          "</tr>"
                          "</table>"
                          "</form>"
                          "<script>"
                          "function check(form)"
                          "{"
                          "if(form.userid.value=='user' && form.pwd.value=='pass')"
                          "{"
                          "window.open('/index_html')"
                          "}"
                          "else"
                          "{"
                          " alert('Error Password or Username')/*displays error message*/"
                          "}"
                          "}"
                          "</script>"
                          "</body>",
                          -1);

    return ESP_OK;
}

static esp_err_t http_resp_dir_html(httpd_req_t *req, const char *dirpath)
{
    int signal_level[20] = {0};
    scann();
    //  char signal[] = "0";

    /* Send HTML file header */
    httpd_resp_send_chunk(req,
                          "<!DOCTYPE html>"
                          "<html>"
                          "<head>"
                          "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /> "
                          "<style>"
                          "body {margin:0;}"

                          "ul {"
                          "  list-style-type: none;"
                          "  margin: 0;"
                          "  padding: 0;"
                          "  overflow: hidden;"
                          "  background-color: #333;"
                          "  position: fixed;"
                          "  top: 0;"
                          "  width: 100%;"
                          "}"
                          "li {"
                          "  float: left;"

                          "}"

                          "li:last-child {"
                          "  border-right: none;"
                          "}"

                          "li a {"
                          "  display: block;"
                          "  color: white;"
                          "  text-align: center;"
                          "  padding: 14px 16px;"
                          "  text-decoration: none;"
                          "}"

                          "li a:hover:not(.active) {"
                          "  background-color: #111;"
                          "}"

                          ".active {"
                          "  background-color: #e00a0a;"
                          "}"
                          "</style>"
                          "</head>"
                          "<head>"
                          "<style>"
                          "* {"
                          "  box-sizing: border-box;"
                          "}"

                          "input[type=text], select, textarea {"
                          "  width: 100%;"
                          "  padding: 5px 5px;"
                          "  border: 1px solid #ccc;"
                          "  border-radius: 4px;"
                          "  resize: vertical;"
                          "}"

                          "label {"
                          "  padding: 12px 12px 12px 0;"
                          "  display: inline-block;"
                          "}"

                          "input[type=submit] {"
                          "  background-color: #e00a0a;"
                          "  color: white;"
                          "  padding: 12px 20px;"
                          "  border: none;"
                          "  border-radius: 4px;"
                          "  cursor: pointer;"
                          "  float: right;"
                          "}"

                          "input[type=submit]:hover {"
                          "  background-color: #380a0a;"
                          "}"

                          ".container {"
                          "  border-radius: 5px;"
                          "  background-color: #f2f2f2;"
                          "  padding: 20px;"
                          "}"

                          ".col-25 {"
                          "  float: left;"
                          "  width: 25%;"
                          "  margin-top: 6px;"
                          "}"
                          ".col-15 {"
                          "  float: left;"
                          "  width: 15%;"
                          "  margin-top: 6px;"
                          "}"
                          ".col-75 {"
                          "  float: left;"
                          "  width: 75%;"
                          "  margin-top: 6px;"
                          "}"
                          ".col-55 {"
                          "  float: left;"
                          "  width: 55%;"
                          "  margin-top: 6px;"
                          "}"
                          "/* Clear floats after the columns */"
                          ".row:after {"
                          "  content: \"\";"
                          "  display: table;"
                          "  clear: both;"
                          "}"
                          /* Responsive layout - when the screen is less than 600px wide, make the two columns stack on top of each other instead of next to each other */
                          "@media screen and (max-width: 481px) {"
                          "  .col-25, .col-75, input[type=submit] {"
                          "    width: 100%;"
                          "    margin-top: 0;"
                          "  }"
                          "}"
                          "</style>"
                          "</head>"
                          "<body>"
                          "<ul>"
                          "  <li><a class=\"active\" href=\"?home\">Connections</a></li>"
                          "  <li><a href=\"?contact\">Contact</a></li>"
                          "  <li><a href=\"?about\">About</a></li> "
                          " <li style=\"float:right\"><a href=\"?about\"> <img src=\"logo2.png\" width=\"70\" height=\"14\" /></a></li>"
                          "</ul>"
                          "<div style=\"padding:20px;margin-top:50px;\" class=\"container\">"
                          "  <form action=\"/index_html\">"
                          "<h2>Wi-Fi Configuration</h2>"
                          "<p>This part is used to change the network that gateway will connect. Select the right SSID from drop-down selections and write password for this network. </p>"
                          "  <div class=\"row\">"
                          "    <div class=\"col-25\">"
                          "      <label for=\"ssid\">SSID</label>"
                          "    </div>"
                          "    <div class=\"col-15\">"
                          "      <select id=\"ssid\" name=\"ssid\">",
                          -1);
    for (int i = 0; i < ap_num; i++)
    {
        strcpy(size_ssid, (char *)ap_records[i].ssid);
        httpd_resp_send_chunk(req, "    <option value='", -1);
        httpd_resp_send_chunk(req, (char *)ap_records[i].ssid, strlen(size_ssid));
        httpd_resp_send_chunk(req, "'>", -1);
        httpd_resp_send_chunk(req, (char *)ap_records[i].ssid, strlen(size_ssid));
        httpd_resp_send_chunk(req, " signal level:  ", -1);
        if (ap_records[i].rssi >= -55)
        {
            httpd_resp_send_chunk(req, " 5 ", -1);
        }
        else if (ap_records[i].rssi < -55 && ap_records[i].rssi >= -65)
        {
            httpd_resp_send_chunk(req, " 4 ", -1);
        }
        else if (ap_records[i].rssi < -65 && ap_records[i].rssi >= -75)
        {
            httpd_resp_send_chunk(req, " 3", -1);
        }
        else if (ap_records[i].rssi < -75 && ap_records[i].rssi >= -85)
        {
            httpd_resp_send_chunk(req, " 2 ", -1);
        }
        else if (ap_records[i].rssi < -85)
        {
            httpd_resp_send_chunk(req, " 1", -1);
        }
        else
        {
            httpd_resp_send_chunk(req, " 0", -1);
        }

        httpd_resp_send_chunk(req, "</option>", -1);
    }
    httpd_resp_send_chunk(req,
                          "      </select>"
                          "    </div>"
                          "  </div>"

                          "<div class=\"row\">"
                          "    <div class=\"col-25\">"
                          "      <label for=\"pass\">Password</label>"
                          "    </div>"
                          "    <div class=\"col-15\">"
                          "      <input type=\"text\" id=\"fname\" name=\"pass\" placeholder=\"Your password..\">"
                          "    </div>"
                          "  </div>"
                          "  <br>"
                          "  <div class=\"row\">"
                          "    <input type=\"submit\" value=\"Submit\">"
                          "  </div>"
                          "  </form>"
                          "  Storage:  ",
                          -1);
    httpd_resp_send_chunk(req, ssid_get, strlen(ssid_get));
    httpd_resp_send_chunk(req, "  Pass:  ", -1);
    httpd_resp_send_chunk(req, pass_get, strlen(pass_get));
    httpd_resp_send_chunk(req,
                          "</div>"
                          "<br>"

                          "<div class=\"container\">"
                          "  <form action=\"/index_html\">"

                          "<h2>Connection Type</h2>"
                          "<p>Choose the right option for connection type.</p>"

                          "  <div class=\"row\">"
                          "    <div class=\"col-25\">"
                          "      <label for=\"conn\">Type:</label>"
                          "    </div>"
                          "    <div class=\"col-15\">"
                          "  <select id='connections' name='connections'>"
                          "    <option value='wifi'>wifi</option>"
                          "    <option value='gsm'>Gsm</option>"
                          "    <option value='zigbee'>Zigbee</option>"
                          "  </select>"
                          "    </div>"
                          "  </div>"

                          "  <br>"
                          "  <div class=\"row\">"
                          "    <input type=\"submit\" value=\"Submit\">"
                          "  </div>"
                          "  </form>",
                          -1);
    httpd_resp_send_chunk(req, " Storage:  ", -1);
    httpd_resp_send_chunk(req, conn_get, strlen(conn_get));
    httpd_resp_send_chunk(req, "</div>"

                               "</body>"

                               "</html>",
                          -1);

    return ESP_OK;
}

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf"))
    {
        return httpd_resp_set_type(req, "application/pdf");
    }
    else if (IS_FILE_EXT(filename, ".html"))
    {
        return httpd_resp_set_type(req, "text/html");
    }
    else if (IS_FILE_EXT(filename, ".png"))
    {
        return httpd_resp_set_type(req, "image/png");
    }
    else if (IS_FILE_EXT(filename, ".jpeg"))
    {
        return httpd_resp_set_type(req, "image/jpeg");
    }
    else if (IS_FILE_EXT(filename, ".ico"))
    {
        return httpd_resp_set_type(req, "image/x-icon");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

/* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
static const char *get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{

    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest)
    {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash)
    {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize)
    {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }
    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

/* Handler to download a file kept on the server */
static esp_err_t webserver_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;

    for (int a = 0; a < 5; a++)
    {
        strcpy(parsed[a], "");
    }
    ESP_LOGI(TAG, " storage ssid: %s     pass: %s   ", ssid_get, pass_get);

    const char *filename = get_path_from_uri(filepath, ((struct file_server_data *)req->user_ctx)->base_path,
                                             req->uri, sizeof(filepath));

    printf("uri %s \n", req->uri);
    strcpy(parse, req->uri); // calling strcpy function

    printf("parse %s ,   filename:  %s\n", parse, filename);

    if (!filename)
    {
        ESP_LOGE(TAG, "Filename is too long");
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }

    /* If name has trailing '/', respond with directory contents */
    if (filename[strlen(filename) - 1] == '/')
    {
        return ask_resp_dir_html(req, filepath);
    }
    if (stat(filepath, &file_stat) == -1)
    {
        /* If file not present on SPIFFS check if URI
         * corresponds to one of the hardcoded paths */
        if (strcmp(filename, "/index_html") == 0)
        {
            ESP_LOGI(TAG, "inside index_html");
            if (!(strcmp(parse, "/index_html")))
            {
                return http_resp_dir_html(req, filepath);
            }
            else
                return connect_get_handler(req);
        }
        else if (strcmp(filename, "/contact") == 0)
        {
            ESP_LOGI(TAG, "inside contact");
            if (!(strcmp(parse, "/contact")))
            {
                return contact_resp_dir_html(req, filepath);
            }
            else
                return connect_get_handler(req);
        }
        else if (strcmp(filename, "/about") == 0)
        {
            ESP_LOGI(TAG, "inside contact");
            if (!(strcmp(parse, "/about")))
            {
                return about_resp_dir_html(req, filepath);
            }
            else
                return connect_get_handler(req);
        }
        else if (strcmp(filename, "/logo.png") == 0)
        {
            ESP_LOGI(TAG, "logo!");
            return logo_get_handler(req);
        }
        else if (strcmp(filename, "/facebook.png") == 0)
        {
            ESP_LOGI(TAG, "facebook!");
            return facebook_get_handler(req);
        }
        else if (strcmp(filename, "/logo2.png") == 0)
        {
            ESP_LOGI(TAG, "logo2!");
            return logo2_get_handler(req);
        }
        else if (strcmp(filename, "/instagram.png") == 0)
        {
            ESP_LOGI(TAG, "instagram!");
            return instagram_get_handler(req);
        }
        else if (strcmp(filename, "/linkedin.png") == 0)
        {
            ESP_LOGI(TAG, "linkedin!");
            return linkedin_get_handler(req);
        }
        else if (strcmp(filename, "/location.png") == 0)
        {
            ESP_LOGI(TAG, "location!");
            return location_get_handler(req);
        }
        else if (strcmp(filename, "/mail.png") == 0)
        {
            ESP_LOGI(TAG, "mail!");
            return mail_get_handler(req);
        }
        else if (strcmp(filename, "/mobile.png") == 0)
        {
            ESP_LOGI(TAG, "mobile!");
            return mobile_get_handler(req);
        }
        else if (strcmp(filename, "/web.png") == 0)
        {
            ESP_LOGI(TAG, "web!");
            return web_get_handler(req);
        }
        else if (strcmp(filename, "/youtube.png") == 0)
        {
            ESP_LOGI(TAG, "youtube!");
            return youtube_get_handler(req);
        }
        ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
        /* Respond with 404 Not Found */
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
        return ESP_FAIL;
    }

    fd = fopen(filepath, "r");
    if (!fd)
    {
        ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", filename, file_stat.st_size);
    set_content_type_from_file(req, filename);

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *chunk = ((struct file_server_data *)req->user_ctx)->scratch;
    size_t chunksize;
    do
    {
        /* Read file in chunks into the scratch buffer */
        chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);

        /* Send the buffer contents as HTTP response chunk */
        if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK)
        {

            fclose(fd);
            ESP_LOGE(TAG, "File sending failed!");
            /* Abort sending file */
            httpd_resp_sendstr_chunk(req, NULL);
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            return ESP_FAIL;
        }

        /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    /* Close file after sending complete */
    fclose(fd);
    ESP_LOGI(TAG, "File sending complete");

    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* Function to start the file server */
esp_err_t start_file_server(const char *base_path)
{
    static struct file_server_data *server_data = NULL;
    /* Validate file storage base path */
    if (!base_path || strcmp(base_path, "/spiffs") != 0)
    {
        ESP_LOGE(TAG, "File server presently supports only '/spiffs' as base path");
        return ESP_ERR_INVALID_ARG;
    }
    if (server_data)
    {
        ESP_LOGE(TAG, "File server already started");
        return ESP_ERR_INVALID_STATE;
    }
    /* Allocate memory for server data */
    server_data = calloc(1, sizeof(struct file_server_data));
    if (!server_data)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return ESP_ERR_NO_MEM;
    }
    strlcpy(server_data->base_path, base_path,
            sizeof(server_data->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }

    /* URI handler for getting uploaded files */
    httpd_uri_t file_server = {
        .uri = "/*", // Match all URIs of type /path/to/file
        .method = HTTP_GET,
        .handler = webserver_get_handler,
        .user_ctx = server_data // Pass server data as context
    };
    httpd_register_uri_handler(server, &file_server);

    return ESP_OK;
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();

    // initialize the wifi event handler
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    // configure, initialize and start the wifi driver
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_config_t ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .password = AP_PASSWORD,
            .ssid_hidden = 0, // Access point in ssid sini wifi scanlarda gizlemek için kullanılan parametre
        },
    };
    ap_config.ap.authmode = WIFI_AUTH_OPEN;
    ap_config.ap.ssid_len = 0;
    ap_config.ap.max_connection = AP_MAX_CONN;
    ap_config.ap.channel = AP_CHANNEL;

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));

    wifi_config_t sta_config = {
        .sta = {
            .ssid = STA_SSID,
            .password = STA_PASSWORD},
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));

    ESP_ERROR_CHECK(esp_wifi_start()); //STA
    printf("Starting access point, SSID=%s\n", "Test_Transmitter");
    ESP_ERROR_CHECK(esp_wifi_connect()); //AP
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */

    /* Initialize file storage */
    ESP_ERROR_CHECK(init_spiffs());
    ESP_ERROR_CHECK(start_file_server("/spiffs"));
}
