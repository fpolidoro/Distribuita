enum operation {
	ENCODE  = 0,
	DECODE = 1
};
struct Request {
    float data<>;		/* the request data */
    operation op;		/* the requested operation */
};
struct Response {
    bool success;	 /* true if the operation has been executed successfully */
    float data<>;  /* the response data, empty in case of unsuccessful 
    operation */
};
