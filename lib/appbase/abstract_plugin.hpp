namespace appbase {
  class AbstractPlugin {
  public:
    virtual void startup() = 0;
    virtual void shutdown() = 0;
  };
}
