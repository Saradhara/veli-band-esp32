# velibend backend

### API: {{url}}/api/token/ supported POST, return Auth JWT token

##### POST request json structure to JWT token
```
{
	"username":"qual5_operator",
	"password":"srinivasan"
}
``` 
##### POST response JWT json structure with response code 201 
```
{
    "refresh": "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI2NjUzYWEwZTI4MTg0MjBhYmU3YjQwYTg2ZTQwNWJlNCIsInVzZXJfaWQiOjIsImV4cCI6MTU5ODM4MzkyNSwidG9rZW5fdHlwZSI6InJlZnJlc2gifQ.eXFch15h7PDukEztVhUzH-JLj0Q2Ym3BcldOM5VTI2w",
    "access": "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJqdGkiOiI2NjU4M2U3MmNlMTA0YzY2YTQ0ZWNhNDBlNzdhNTMxMyIsInVzZXJfaWQiOjIsImV4cCI6MTU5ODI5NzgyNSwidG9rZW5fdHlwZSI6ImFjY2VzcyJ9.exsgZe6cFN0tIdQN-kgPKZwYLClmtpwMTZIuUL8doC0"
}
```
##### Note : All the API are secure and can be accessed only with valid JWT token
```
GET /bulk-bag-events/?hub_id=2012-RC HTTP/1.1
Host: localhost:8000
Authorization: Bearer {{token}} All the request required this 
``` 