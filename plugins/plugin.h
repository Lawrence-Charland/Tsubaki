#ifndef _PLUGIN_CORE_H_
#define _PLUGIN_CORE_H_

template <typename T> class Plugin {
    Plugin &operator=(const Plugin &) = delete;
    Plugin(const Plugin &) = delete;

  protected:
    Plugin() = default;
    virtual ~Plugin() = default;

  public:
    static T &GetInstance() {
        static T instance;
        return instance;
    }
};

#endif // !_PLUGIN_CORE_H_
