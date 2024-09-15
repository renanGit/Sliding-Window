# docker build -t sw_cli:1 -f .\Client\Dockerfile .

FROM gcc:14.2.0 as builder

RUN apt-get update && apt-get install -y cmake

WORKDIR /app
COPY ./Sliding-Window/client.cpp .
COPY ./Sliding-Window/send_recv.* /app/
COPY ./Client/CMakeLists.txt .
RUN mkdir build
RUN cd build && cmake .. && cmake --build .

# To reduce image size
# ---
# FROM scratch
# COPY --from=builder /app/build/client .
# ENTRYPOINT [ "/client", "10.0.0.0", "3000" ]
# ---

# For documentation only, need to use "-p 3000:3000" on run
EXPOSE 3000