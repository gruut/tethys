#pragma once

namespace gruut {
namespace admin_plugin {

template <typename T>
class CommandDelegator {
public:
  CommandDelegator() = default;

  void delegate() {
    setControlType();
    nlohmann::json control_command = createControlCommand();

    sendCommand(control_command);
  }

  const int getControlType() const {
    return control_type;
  }

private:
  virtual nlohmann::json createControlCommand() = 0;
  virtual void sendCommand(nlohmann::json control_command) = 0;
  virtual void setControlType() = 0;

protected:
  int control_type;
};

class LoginCommandDelegator : public CommandDelegator<ReqLogin> {
public:
  LoginCommandDelegator(string_view _env_sk_pem, string_view _cert, string_view _pass)
      : secret_key(_env_sk_pem), cert(_cert), pass(_pass) {}

private:
  nlohmann::json createControlCommand() override {
    nlohmann::json control_command;

    control_command["type"] = getControlType();
    control_command["enc_sk"] = secret_key;
    control_command["cert"] = cert;
    control_command["pass"] = pass;

    return control_command;
  }

  void sendCommand(nlohmann::json control_command) override {
    app().getChannel<incoming::channels::net_control>().publish(control_command);
  }

  void setControlType() override {
    control_type = static_cast<int>(ControlType::LOGIN);
  }

  string secret_key, cert, pass;
};

class StartCommandDelegator : public CommandDelegator<ReqStart> {
public:
  StartCommandDelegator(RunningMode _net_mode) : net_mode(_net_mode) {}

private:
  nlohmann::json createControlCommand() override {
    nlohmann::json control_command;

    control_command["type"] = getControlType();
    control_command["mode"] = net_mode;

    return control_command;
  }

  void sendCommand(nlohmann::json control_command) override {
    app().getChannel<incoming::channels::net_control>().publish(control_command);
  }

  void setControlType() override {
    control_type = static_cast<int>(ControlType::START);
  }

  RunningMode net_mode;
};

} // namespace admin_plugin
}
