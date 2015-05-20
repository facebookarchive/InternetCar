/*
 *  Copyright (c) 2015, Parse, LLC. All rights reserved.
 *
 *  You are hereby granted a non-exclusive, worldwide, royalty-free license to use,
 *  copy, modify, and distribute this software in source code or binary form for use
 *  in connection with the web services and APIs provided by Parse.
 *
 *  As with any software that integrates with the Parse platform, your use of
 *  this software is subject to the Parse Terms of Service
 *  [https://www.parse.com/about/terms]. This copyright notice shall be
 *  included in all copies or substantial portions of the software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 *  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 *  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "car.h"

/* This is the Parse client handle used by the SDK */
ParseClient parseClient = NULL;

#define MAX_MSG_SIZE 128
#define MAX_TYPE_SIZE 20

static char userObjectId[OBJECT_ID_MAX_LEN+1];
static char installationObjectId[OBJECT_ID_MAX_LEN+1];
static char modelObjectId[OBJECT_ID_MAX_LEN+1];
static char queryResults[400];
static char objectJson[1024];

// Cloud Car definitions
#define LEFT_CONTROL_PIN1 6
#define LEFT_CONTROL_PIN2 7
#define RIGHT_CONTROL_PIN1 23
#define RIGHT_CONTROL_PIN2 28

#define PORT_LC1 GPIOA0_BASE
#define PORT_LC2 GPIOA0_BASE
#define PORT_RC1 GPIOA2_BASE
#define PORT_RC2 GPIOA3_BASE

#define GPIO_PIN_LC1 0x40
#define GPIO_PIN_LC2 0x80
#define GPIO_PIN_RC1 0x80
#define GPIO_PIN_RC2 0x10

static int period = 5000000;

static void carGetInstallationObjectIdByIdCallback(ParseClient client, int error, int httpStatus, const char* httpResponseBody) {
    memset(installationObjectId, 0, sizeof(installationObjectId));

    if ((error == 0) && (httpStatus >= 200 && httpStatus < 300)) {
        memset(queryResults, 0, sizeof(queryResults));
        short status = simpleJson(httpResponseBody, "results", queryResults, sizeof(queryResults));
        if ((status != FALSE) && (queryResults[1] == '{')) {
            simpleJson(queryResults+1, "objectId", installationObjectId, sizeof(installationObjectId));
        }
    }
}

void carGetInstallationObjectId() {
    memset(installationObjectId, 0, sizeof(installationObjectId));

    // Encode the query parameter value
    char content[150];
    sprintf(content, "{\"installationId\": \"%s\"}", g_InstallationID);

    char encodedContent[150];
    memset(encodedContent, 0, sizeof(encodedContent));
    urlEncode(content, encodedContent);

    memset(content, 0, sizeof(content));
    sprintf(content, "where=%s", encodedContent);

    parseSendRequest((ParseClient)parseClient, "GET", "/1/installations", content, carGetInstallationObjectIdByIdCallback);
}

static void carGetModelObjectIdByNameCallback(ParseClient client, int error, int httpStatus, const char* httpResponseBody) {
    memset(modelObjectId, 0, sizeof(modelObjectId));

    if ((error == 0) && (httpStatus >= 200 && httpStatus < 300)) {
        memset(queryResults, 0, sizeof(queryResults));
        short status = simpleJson(httpResponseBody, "results", queryResults, sizeof(queryResults));
        if ((status != FALSE) && (queryResults[1] == '{')) {
            simpleJson(queryResults+1, "objectId", modelObjectId, sizeof(modelObjectId));
        }
    }
}

void carGetModelObjectId() {
    memset(modelObjectId, 0, sizeof(modelObjectId));

    // Encode the query parameter value
    char content[150];
    sprintf(content, "{\"appName\": \"fbdr000001a\"}");

    char encodedContent[150];
    memset(encodedContent, 0, sizeof(encodedContent));
    urlEncode(content, encodedContent);

    memset(content, 0, sizeof(content));
    sprintf(content, "where=%s", encodedContent);

    parseSendRequest((ParseClient)parseClient, "GET", "/1/classes/Model", content, carGetModelObjectIdByNameCallback);
}

void carGetUserObjectIdCallback(ParseClient client, int error, int httpStatus, const char* httpResponseBody) {
    memset(userObjectId, 0, sizeof(userObjectId));
    if ((error == 0) && (httpStatus >= 200 && httpStatus < 300)) {
        simpleJson(httpResponseBody, "objectId", userObjectId, OBJECT_ID_MAX_LEN);
    }
}

void carGetUserObjectId() {
    memset(userObjectId, 0, sizeof(userObjectId));
    parseSendRequest(parseClient, "GET", "/1/users/me", NULL, carGetUserObjectIdCallback);
}

void carRequestCallback(ParseClient client, int error, int httpStatus, const char* httpResponseBody) {
    if (httpResponseBody) {
        UART_PRINT("[Blink] Log response: %s\r\n", httpResponseBody);
    }
}

void carUpdateInstallation() {
    carGetInstallationObjectId();

    if (strlen(installationObjectId) > 0) {
        UART_PRINT("[Blink] Updating installation object\r\n");

        carGetModelObjectId();
        carGetUserObjectId();

        char path[40];
        memset(path, 0, sizeof(path));
        sprintf(path, "/1/installations/%s", installationObjectId);

        memset(objectJson, 0, sizeof(objectJson));
        sprintf(objectJson, "{\"deviceName\": \"%s\", \"deviceSubtype\": \"fluffy\", \"model\": {\"__type\":\"Pointer\",\"className\":\"Model\",\"objectId\":\"%s\"}, \"owner\": {\"__type\":\"Pointer\",\"className\":\"_User\",\"objectId\":\"%s\"}}",
                g_DeviceName, modelObjectId, userObjectId);

        parseSendRequest(parseClient, "PUT", path, objectJson, carRequestCallback);
    }
}

void initParseClient() {
    parseClient = parseInitialize(g_ApplicationID, g_ClientKey);

    if (parseGetInstallationId(parseClient) == NULL) {
        parseSetInstallationId(parseClient, g_InstallationID);
    }

    if (parseGetSessionToken(parseClient) == NULL) {
        parseSetSessionToken(parseClient, g_SessionToken);
    }

    carUpdateInstallation();
}

void resetParseClient() {
    parseSetSessionToken(parseClient, NULL);
    parseSetInstallationId(parseClient, NULL);
}

void saveLedState() {
    UART_PRINT("[Blink] Saving LED state.\r\n");

    // TODO: Use parseGetInstallationId() instead of installationObjectId
    sprintf(objectJson, "{\"installationId\": \"%s\", \"value\": {\"state\": \"%s\"}, \"alarm\": true, \"ACL\":{ \"%s\": { \"read\": true, \"write\": true}}}",
            installationObjectId, getLedState(), userObjectId);
    parseSendRequest(parseClient, "POST", "/1/classes/Event", objectJson, carRequestCallback);
}

void carPushNotificationCallback(ParseClient client, int error, const char* data) {
    if (error != 0) {
        UART_PRINT("[Blink] Push error: %d \r\n", error);
    } else {
        UART_PRINT("[Blink] Push notification: %s \r\n", data);

        char msg[MAX_MSG_SIZE];
        char msgType[MAX_TYPE_SIZE];

        simpleJson(data, "data", msg, MAX_MSG_SIZE);
        simpleJson(msg, "alert", msgType, MAX_TYPE_SIZE);

        UART_PRINT("[Blink] push: %s\r\n", data);

        setLedState(msgType);
        if (strcmp("off", msgType) == 0) {
        	UART_PRINT("Drive!!!\r\n");
            GPIO_IF_Set(LEFT_CONTROL_PIN1, PORT_LC1, GPIO_PIN_LC1, 0);
            GPIO_IF_Set(LEFT_CONTROL_PIN2, PORT_LC2, GPIO_PIN_LC2, 1);
            GPIO_IF_Set(RIGHT_CONTROL_PIN1, PORT_RC1, GPIO_PIN_RC1, 0);
            GPIO_IF_Set(RIGHT_CONTROL_PIN2, PORT_RC2, GPIO_PIN_RC2, 1);
            //UART_PRINT("set!!!\r\n");

            UtilsDelay(period); // 750 ms

            GPIO_IF_Set(LEFT_CONTROL_PIN1, PORT_LC1, GPIO_PIN_LC1, 0);
            GPIO_IF_Set(LEFT_CONTROL_PIN2, PORT_LC2, GPIO_PIN_LC2, 0);
            GPIO_IF_Set(RIGHT_CONTROL_PIN1, PORT_RC1, GPIO_PIN_RC1, 0);
            GPIO_IF_Set(RIGHT_CONTROL_PIN2, PORT_RC2, GPIO_PIN_RC2, 0);
            //UART_PRINT("unset!!!\r\n");
        } else if (strcmp("on", msgType) == 0) {
        	UART_PRINT("Reverse!!!\r\n");
            GPIO_IF_Set(LEFT_CONTROL_PIN1, PORT_LC1, GPIO_PIN_LC1, 1);
            GPIO_IF_Set(LEFT_CONTROL_PIN2, PORT_LC2, GPIO_PIN_LC2, 0);
            GPIO_IF_Set(RIGHT_CONTROL_PIN1, PORT_RC1, GPIO_PIN_RC1, 1);
            GPIO_IF_Set(RIGHT_CONTROL_PIN2, PORT_RC2, GPIO_PIN_RC2, 0);
            //UART_PRINT("set!!!\r\n");

            UtilsDelay(period); // 750 ms

            GPIO_IF_Set(LEFT_CONTROL_PIN1, PORT_LC1, GPIO_PIN_LC1, 0);
            GPIO_IF_Set(LEFT_CONTROL_PIN2, PORT_LC2, GPIO_PIN_LC2, 0);
            GPIO_IF_Set(RIGHT_CONTROL_PIN1, PORT_RC1, GPIO_PIN_RC1, 0);
            GPIO_IF_Set(RIGHT_CONTROL_PIN2, PORT_RC2, GPIO_PIN_RC2, 0);
            //UART_PRINT("unset!!!\r\n");
        }

        /*
        if (setLedState(msgType)) {
            saveLedState();
        } else if (strcmp("deprovisioning", msgType) == 0) {
            factoryReset();
        } else {
            UART_PRINT("[Blink] Unknown notification: %s\r\n", data);
        }*/
    }
}

void startPushService(void *pvParameters) {
    if (parseClient) {
        parseSetPushCallback(parseClient, carPushNotificationCallback);
        parseStartPushService(parseClient);

        parseRunPushLoop(parseClient);

        // If we exit the loop
        UART_PRINT("[Blink] Terminated due to Parse push service error.\r\n");
    } else {
        UART_PRINT("[Blink] Terminated due to Parse initialization error.\r\n");
    }
}

short startPushServiceTask() {
    return osi_TaskCreate(startPushService, (signed char *)"PushService", OSI_STACK_SIZE, NULL, OOB_TASK_PRIORITY, NULL);
}
