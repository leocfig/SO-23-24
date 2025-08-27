#ifndef PARSER_REQUESTS_H
#define PARSER_REQUESTS_H

/// Parses the request setup from the client.
/// @param rx The server's pipe filedescriptor to read the request from.
/// @param req_pipe_path The pointer to store the client's request pipe path.
/// @param resp_pipe_path The pointer to store the client's request pipe path.
/// @return 0 if parsing was successfully made, 1 otherwise.
int parse_setup(int rx, char *req_pipe_path, char *resp_pipe_path);

/// Parses a request for the command create. 
/// @param req_pipe The client's request pipe filedescriptor to read from.
/// @param event_id The variable to store the event ID to be created.
/// @param num_rows The variable to store the number of rows of the event to be created.
/// @param num_cols The variable to store the number of columns of the event to be created.
/// @return 0 if the parsing was successfully made, 1 otherwise.
int parse_create(int req_pipe, unsigned int *event_id, size_t *num_rows, size_t *num_cols);

/// @brief 
/// @param req_pipe The client's request pipe filedescriptor to read from.
/// @param event_id The variable to store the event ID to reserve the seats.
/// @param num_seats The variable to store the number of seats to reserve.
/// @param xs The pointer to store the coordinate X of the seats to reserve.
/// @param ys The pointer to store the coordinate Y of the seats to reserve.
/// @return 0 if the parsing was successfully made, 1 otherwise.
int parse_reserve(int req_pipe, unsigned int *event_id, 
                  size_t *num_seats, size_t *xs, size_t *ys);

/// Parses a request for the command show. 
/// @param req_pipe The client's request pipe filedescriptor to read from.
/// @param event_id The variable to store the event ID to show
/// @return 0 if the parsing was successfully made, 1 otherwise.
int parse_show(int req_pipe, unsigned int *event_id);

#endif