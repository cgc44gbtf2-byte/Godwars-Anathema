"""Bounded wire decoder for synthetic GMCP smoke tests, not a GUI client."""
import json


class GMCPProbe:
    def __init__(self):
        self.state = 0
        self.verb = 0
        self.payload = bytearray()
        self.offers = self.resets = 0
        self.records = []

    def feed(self, data):
        for byte in data:
            if self.state == 0:
                if byte == 255:
                    self.state = 1
            elif self.state == 1:
                if byte in (251, 252, 253, 254):
                    self.verb, self.state = byte, 2
                elif byte == 250:
                    self.payload.clear()
                    self.state = 3
                else:
                    self.state = 0
            elif self.state == 2:
                if byte == 201:
                    self.offers += self.verb == 251
                    self.resets += self.verb == 252
                self.state = 0
            elif self.state == 3:
                if byte == 255:
                    self.state = 4
                else:
                    self.payload.append(byte)
            else:
                if byte == 240:
                    if self.payload and self.payload[0] == 201:
                        package, raw = self.payload[1:].decode("utf-8").split(" ", 1)
                        self.records.append((package, json.loads(raw)))
                        assert len(self.records) < 1000, "Unexpected GMCP flood"
                    self.state = 0
                else:
                    assert byte == 255, "Malformed GMCP escape"
                    self.payload.append(255)
                    self.state = 3
            assert len(self.payload) <= 8192, "Oversized GMCP packet"
