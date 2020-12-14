#!/usr/bin/env python
# Copyright 2020 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""Simple example script that uses pw_rpc."""

import argparse
import os
from pathlib import Path

import serial  # type: ignore
from time import sleep
import threading
from queue import Queue

from pw_hdlc_lite.rpc import HdlcRpcClient

# Point the script to the .proto file with our RPC services.
PROTO = Path(os.environ['PW_ROOT'], 'pw_rpc/pw_rpc_protos/echo.proto')
PROTO2 = Path('examples/pigweed-app/nrfconnect/main/pigweed_test.proto')

def thread_func(q : Queue, chip_service):
    q.get()
    print("in THREAD")
    status, payload = chip_service.SendMessage(transportnumber=33, packet=b'hello')

def script(device: str, baud: int) -> None:
    # Set up a pw_rpc client that uses HDLC.
    ser = serial.Serial(device, baud, timeout=0.01)
    client = HdlcRpcClient(lambda: ser.read(4096), ser.write, [PROTO, PROTO2])

    # Make a shortcut to the EchoService.
    chip_service = client.rpcs().chip.rpc.TestService

    q = Queue()
    x = threading.Thread(target=thread_func, args=[q, chip_service])
    x.start()


    def cb(x, y, z):
        print ("in callback")
        if (z and z.packet):
            print("Received %s" % z.packet)
            # status, payload = chip_service.SendMessage(transportnumber=33, packet=b'hello')
            # if status.ok():
            #     print('The status of callback SendMessage was', status)
            #     print('The payload was', payload)
            # else:
            #     print('Uh oh, this RPC returned', status)

            q.put(True)

    # status, payload = chip_service.SendMessage(transportnumber=33, packet=b'hello')

    # sleep(4)

    # if status.ok():
    #     print('The status of SendMessage was', status)
    #     print('The payload was', payload)
    # else:
    #     print('Uh oh, this RPC returned', status)

    # Call some RPCs and check the results.
    chip_service.OpenTransport.reinvoke(cb, number=33)

    sleep(4)

    status, payload = chip_service.MessageTest(number=33)
    # sleep(1)
    # status, payload = chip_service.SendMessage(transportnumber=33, packet=b'hello')

    sleep(4)

    if status.ok():
        print('The status of MessageTest was', status)
        print('The payload was', payload)
    else:
        print('Uh oh, this RPC returned', status)

    x.join()

def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--device',
                        '-d',
                        default='/dev/ttyACM0',
                        help='serial device to use')
    parser.add_argument('--baud',
                        '-b',
                        type=int,
                        default=115200,
                        help='baud rate for the serial device')
    script(**vars(parser.parse_args()))


if __name__ == '__main__':
    main()
