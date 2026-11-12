AIKON-VXP test server (chunk 1 counterpart)
=============================================

What this is
-------------
A tiny Go HTTP server with one endpoint, GET /ping, that replies with
plain text. It exists only so NetTest.vxp (the phone-side app) has a
real plain-HTTP server to connect to while proving the basic TCP path
works. No TLS, no auth, nothing production-grade - intentionally.

Deploy to Fly.io (you'll need your own Fly.io account - I can't do
this step for you, it needs your credentials)
--------------------------------------------------------------------
1. Install flyctl: https://fly.io/docs/flyctl/install/
2. fly auth login
3. Edit fly.toml: change "CHANGE-ME-aikon-vxp" to something unique
   (Fly app names are global), and set primary_region if you want a
   different one.
4. From this folder: fly launch --no-deploy   (it'll detect fly.toml
   and the Dockerfile; say no to adding a database or anything else)
5. fly deploy
6. Note the public hostname Fly gives you (prints as
   https://your-app-name.fly.dev, but for THIS server you'll actually
   reach it over plain HTTP on port 80, since the service block maps
   port 80 straight through with no TLS handler)

Then wire up the phone side
-----------------------------
In net_test.c (the AIKON-VXP repo), set:
  #define SERVER_HOST   "your-app-name.fly.dev"
  #define SERVER_PORT   80
  #define SERVER_PATH   "/ping"
and rebuild (make), then re-run the IMSI packaging step once you have
a real SIM in the phone.

Local test without Fly at all
--------------------------------
go run main.go
curl http://localhost:8080/ping
