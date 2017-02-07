// Copyright 2017 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Common/CommonTypes.h"
#include "Core/IOS/Device.h"

namespace IOS
{
namespace HLE
{
namespace USB
{
enum StandardDeviceRequestCodes
{
  REQUEST_GET_DESCRIPTOR = 6,
  REQUEST_SET_CONFIGURATION = 9,
  REQUEST_GET_INTERFACE = 10,
  REQUEST_SET_INTERFACE = 11,
};

enum ControlRequestTypes
{
  DIR_HOST2DEVICE = 0,
  DIR_DEVICE2HOST = 1,
  TYPE_STANDARD = 0,
  TYPE_VENDOR = 2,
  REC_DEVICE = 0,
  REC_INTERFACE = 1,
};

constexpr u16 USBHDR(u8 dir, u8 type, u8 recipient, u8 request)
{
  return static_cast<u16>(((dir << 7 | type << 5 | recipient) << 8) | request);
}

struct DeviceDescriptor
{
  u8 bLength;
  u8 bDescriptorType;
  u16 bcdUSB;
  u8 bDeviceClass;
  u8 bDeviceSubClass;
  u8 bDeviceProtocol;
  u8 bMaxPacketSize0;
  u16 idVendor;
  u16 idProduct;
  u16 bcdDevice;
  u8 iManufacturer;
  u8 iProduct;
  u8 iSerialNumber;
  u8 bNumConfigurations;
};

struct ConfigDescriptor
{
  u8 bLength;
  u8 bDescriptorType;
  u16 wTotalLength;
  u8 bNumInterfaces;
  u8 bConfigurationValue;
  u8 iConfiguration;
  u8 bmAttributes;
  u8 MaxPower;
};

struct InterfaceDescriptor
{
  u8 bLength;
  u8 bDescriptorType;
  u8 bInterfaceNumber;
  u8 bAlternateSetting;
  u8 bNumEndpoints;
  u8 bInterfaceClass;
  u8 bInterfaceSubClass;
  u8 bInterfaceProtocol;
  u8 iInterface;
};

struct EndpointDescriptor
{
  u8 bLength;
  u8 bDescriptorType;
  u8 bEndpointAddress;
  u8 bmAttributes;
  u16 wMaxPacketSize;
  u8 bInterval;
};

struct TransferCommand
{
  Request ios_request;
  u32 data_address = 0;

  TransferCommand(const Request& ios_request_, u32 data_address_)
      : ios_request(ios_request_), data_address(data_address_)
  {
  }
  virtual ~TransferCommand() = default;
  // Called after a transfer has completed and before replying to the transfer request.
  virtual void OnTransferComplete() const {}
  std::unique_ptr<u8[]> MakeBuffer(size_t size) const;
  void FillBuffer(const u8* src, size_t size) const;
};

struct CtrlMessage : TransferCommand
{
  u8 request_type = 0;
  u8 request = 0;
  u16 value = 0;
  u16 index = 0;
  u16 length = 0;
  using TransferCommand::TransferCommand;
};

struct BulkMessage : TransferCommand
{
  u16 length = 0;
  u8 endpoint = 0;
  using TransferCommand::TransferCommand;
};

struct IntrMessage : TransferCommand
{
  u16 length = 0;
  u8 endpoint = 0;
  using TransferCommand::TransferCommand;
};

struct IsoMessage : TransferCommand
{
  u32 packet_sizes_addr = 0;
  std::vector<u16> packet_sizes;
  u16 length = 0;
  u8 num_packets = 0;
  u8 endpoint = 0;
  using TransferCommand::TransferCommand;
  void SetPacketReturnValue(size_t packet_num, u16 return_value) const;
};

class Device
{
public:
  virtual ~Device();
  u64 GetId() const;
  u16 GetVid() const;
  u16 GetPid() const;
  bool HasClass(u8 device_class) const;
  std::vector<u8> GetDescriptorsUSBV4() const;
  std::vector<u8> GetDescriptorsUSBV5(u8 interface, u8 alt_setting) const;

  virtual DeviceDescriptor GetDeviceDescriptor() const = 0;
  virtual std::vector<ConfigDescriptor> GetConfigurations() const = 0;
  virtual std::vector<InterfaceDescriptor> GetInterfaces(u8 config) const = 0;
  virtual std::vector<EndpointDescriptor> GetEndpoints(u8 config, u8 interface, u8 alt) const = 0;

  virtual std::string GetErrorName(int error_code) const;
  virtual bool Attach(u8 interface) = 0;
  virtual int CancelTransfer(u8 endpoint) = 0;
  virtual int ChangeInterface(u8 interface) = 0;
  virtual int GetNumberOfAltSettings(u8 interface) = 0;
  virtual int SetAltSetting(u8 alt_setting) = 0;
  virtual int SubmitTransfer(std::unique_ptr<CtrlMessage> message) = 0;
  virtual int SubmitTransfer(std::unique_ptr<BulkMessage> message) = 0;
  virtual int SubmitTransfer(std::unique_ptr<IntrMessage> message) = 0;
  virtual int SubmitTransfer(std::unique_ptr<IsoMessage> message) = 0;

protected:
  std::vector<u8> GetDescriptors(std::function<bool(const InterfaceDescriptor&)> predicate) const;
  u64 m_id = 0xFFFFFFFFFFFFFFFF;
};
}  // namespace USB
}  // namespace HLE
}  // namespace IOS