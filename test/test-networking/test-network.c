#include <cutter.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <network.h>
#include <errors.h>
#include <helperFunctions.h>

extern int sockfd;

/**
 * Test for network_connect.
 * It works by testing some exceptional cases, then by creating a new process
 * that opens a socket and connecting to it. Some data is transfered back and
 * forth between the 2 processes using the opened socket.
 *
 * This function assumes the use of sockfd private variable in the networking
 * module.
 */
void test_network_connect() {
    cut_assert_not_equal_int(0, network_connect("", 8080),
                             "Connection to empty hostname succeeded");
    cut_assert_not_equal_int(0, network_connect("abcd", 8080),
                             "Connection to non-existent host succeeded");
    cut_assert_not_equal_int(0, network_connect("localhost", -1),
                             "Connection to port -1 succeeded");
    cut_assert_not_equal_int(0, network_connect("localhost", 70000),
                             "Connection to port 70000 succeeded");

    int pid = fork();
    if (pid == 0) {
        int newsockfd = openLocalhostSocket(8080);

        write(newsockfd, "test", 5);

        char buffer[10];
        read(newsockfd, buffer, 10);
        cut_assert_equal_string("check", buffer, "Second data transfer failed");

        close(newsockfd);

        exit(EXIT_SUCCESS);
    }

    sleep(1);
    network_connect("localhost", 8080);
    cut_assert_true(sockfd >= 0,
                    "Network connect failed; negative socket");
    sleep(1);

    char buffer[10];
    read(sockfd, buffer, 10);
    cut_assert_equal_string("test", buffer, "First data transfer failed");

    write(sockfd, "check", 6);

    pid = fork();
    if (pid == 0) {
        int newsockfd = openLocalhostSocket(8081);

        close(newsockfd);

        exit(EXIT_SUCCESS);
    }

    sleep(1);
    cut_assert_not_equal_int(0, network_connect("localhost", 8081),
                             "Reconnection attempt succedeed without "
                             "previous disconnect");
    sockfd = connectToLocalhostSocket(8081);

    close(sockfd);
    sockfd = -1;
}

/**
 * Check id a file descriptor is valid (i.e. the resource is still open).
 * It is used to test the disconnect function.
 */
int fdIsValid(int fd)
{
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

/**
 * Test for network_disconnect.
 * It works by opening a socket in another process, opening it and calling
 * network_disconnect on it.
 *
 * This function assumes the use of sockfd private variable in the networking
 * module.
 */
void test_network_disconnect() {
    int pid = fork();
    if (pid == 0) {
        int newsockfd = openLocalhostSocket(8079);

        close(newsockfd);

        exit(EXIT_SUCCESS);
    }

    sleep(1);
    sockfd = connectToLocalhostSocket(8079);

    network_disconnect();

    cut_assert_false(fdIsValid(sockfd),
                     "Network disconnect failed to disable socket "
                     "file descriptor");
}

/**
 * Test for network_read.
 * It works by testing a exceptional case, then by creating a new process that
 * opens a socket and connecting to it. Some data is transfered from server to
 * client and the client read the data from server using network_read. Then
 * the client checks if the data has been transfered correctly from server.
 * Then are tested some exceptional cases.
 *
 * This function assumes the use of sockfd private variable in the networking
 * module.
 */
void test_network_read() {
    char buffer[10];
    cut_assert_operator_int(0, >=, network_read(buffer, 10),
                            "Read data from non-existent server succeeded");

    int pid = fork();
    if (pid == 0) {
        int newsockfd = openLocalhostSocket(8078);
        write(newsockfd, "test", 5);

        close(newsockfd);

        exit(EXIT_SUCCESS);
    }

    sleep(1);
    sockfd = connectToLocalhostSocket(8078);

    cut_assert_equal_int(5, network_read(buffer, 10),
                         "Not have been read all bytes");
    cut_assert_equal_string("test", buffer, "Data transfer failed");

    cut_assert_operator_int(0, >, network_read(NULL, 0),
                            "Read data succeeded into null string");
    cut_assert_operator_int(0, >, network_read(NULL, 5),
                            "Read data succeeded into null string");
    cut_assert_operator_int(0, >, network_read(buffer, 0),
                            "Read data succeeded into zero-length string");

    close(sockfd);
    sockfd = -1;

    cut_assert_operator_int(0, >=, network_read(buffer, 10),
                            "Read data from non-existent server succeeded");
}

/**
 * Test for network_send.
 * It works by testing a exceptional case, then by creating a new process that
 * opens a socket and connecting to it. Some data is transfered to server using
 * network_send and the server checks if the data has been transfered correctly.
 * Then are tested some exceptional cases.
 *
 * This function assumes the use of sockfd private variable in the networking
 * module.
 */
void test_network_send() {

    cut_assert_not_equal_int(0, network_send("test", 5),
                             "Send data to non-existent server succeeded");

    int pid = fork();
    if (pid == 0) {
        int newsockfd = openLocalhostSocket(8077);
        char buffer[10];
        read(newsockfd, buffer, 10);

        cut_assert_equal_string("test", buffer, "Data transfer failed");

        close(newsockfd);

        exit(EXIT_SUCCESS);
    }

    sleep(1);
    sockfd = connectToLocalhostSocket(8077);

    cut_assert_equal_int(NO_ERROR, network_send("test", 5), "Send data failed");
    cut_assert_not_equal_int(NO_ERROR, network_send(NULL, 0),
                             "Send wrong data succeeded");
    cut_assert_not_equal_int(NO_ERROR, network_send(NULL, 5),
                             "Send wrong data succeeded");
    cut_assert_not_equal_int(NO_ERROR, network_send("test", 0),
                             "Send wrong data succeeded");

    close(sockfd);
    sockfd = -1;

    cut_assert_not_equal_int(0, network_send("test", 5),
                             "Send data to non-existent server succeeded");
}

