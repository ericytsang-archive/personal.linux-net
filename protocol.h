
/**
 * a new client has connected to the chat room.
 */
#define ADD_CLIENT 0

/**
 * an existing client has disconnected to the chat room.
 */
#define RM_CLIENT 1

/**
 * someone has said something in the chat room.
 */
#define SHOW_MSG 2

/**
 * server sends message to client, assigning them their new user name.
 */
#define SET_USR_NAME 3

/**
 * client sends user name they want to use to the server.
 */
#define CHECK_USR_NAME 4
