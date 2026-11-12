FROM golang:1.21-alpine AS build
WORKDIR /src
COPY go.mod main.go ./
RUN go build -o /server main.go

FROM alpine:3.19
COPY --from=build /server /server
EXPOSE 8080
ENTRYPOINT ["/server"]
