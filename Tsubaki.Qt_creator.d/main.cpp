#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <unistd.h>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <cctype>
#include <thread>
#include <signal.h>
#include <atomic>
struct sum_and_dir
{
    std::string type, sum, dir;
    long long int size;
    int mark=0;
    bool mask=0;
    void import(const std::string& line, const bool& is_classed, const int& line_id, const std::string& source)
    {
        if(line.length() == 0)
            throw std::runtime_error(source+": In line "+std::to_string(line_id)+": an empty line is invalid.");
        int i = 0;
        if(is_classed)
        {
            if(line[0] != 'A' && line[0] != 'B')
                throw std::runtime_error(source+": In line "+std::to_string(line_id)+": Each line in stdin must start with 'A ' or 'B '.");
            type = line[0];
            if(line[1] != ' ')
                throw std::runtime_error(source+": In line "+std::to_string(line_id)+": A white space must follow '"+type+"'.");
            i = 2;
        }
        std::string tmp;
        for(; i < line.length() && line[i] != ' '; i++)
            tmp.push_back(line[i]);
        if(i == line.length())
            throw std::runtime_error(source+": In line "+std::to_string(line_id)+": A directory must follow the checksum.");
        if(tmp.length() == 0)
            throw std::runtime_error(source+": In line "+std::to_string(line_id)+": A white space is not allowed before the checksum.");
        if(tmp!="<NONE>")
            for(int j = 0; j < tmp.length(); j++)
                if(!std::isxdigit(tmp[j]))
                    throw std::runtime_error(source+": In line "+std::to_string(line_id)+": '"+tmp+"' is not a checksum.");
        i++;
        sum = tmp;
        for(; i < line.length(); i++)
            dir.push_back(line[i]);
    }
};
namespace cmd
{
struct key_value_pair
{
    std::string key="",value="";
    void import(const std::string& argument)
    {
        int i=0;
        key="";
        for(;i<argument.length()&&argument[i]!='=';i++)
            key.push_back(argument[i]);
        if(i==argument.length())
            return;
        value=argument.substr(i+1,argument.length()-i-1);
    }
};
std::vector<key_value_pair> args;
void init(int init_argc,char** init_argv)
{
    key_value_pair arg;
    std::string tmp;
    for(int i=0;i<init_argc;i++)
    {
        tmp=init_argv[i];
        arg.import(tmp);
        args.push_back(arg);
    }
}
bool find_key(const std::string& target)
{
    key_value_pair arg;
    for(int i=0;i<args.size();i++)
    {
        if(args[i].key==target)
            return 1;
    }
    return 0;
}
int verbose()
{
    if(find_key("-vv"))
        return 2;
    else if(find_key("-v"))
        return 1;
    return 0;
}
std::vector<std::string> get_arr(const std::string& target_key)
{
    std::vector<std::string> arr;
    for(int i=0;i<args.size();i++)
    {
        if(args[i].key==target_key)
            arr.push_back(args[i].value);
    }
    return arr;
}
int get_thd_amount()
{
    if(!cmd::find_key("--thd-amount"))
        return std::thread::hardware_concurrency();
    else
        try
        {
            return std::stoi(cmd::get_arr("--thd-amount")[0]);
        }
        catch(...)
        {
            return std::thread::hardware_concurrency();
        }
}
}
namespace cmp
{
bool cmp_by_sum(const sum_and_dir& A,const sum_and_dir& B)
{
    return A.sum < B.sum;
}
bool cmp_by_dir(const sum_and_dir& A,const sum_and_dir& B)
{
    return A.dir < B.dir;
}
}
namespace file
{
bool B_is_As_subpath(const std::string& A_str, const std::string& B_str)
{
    std::filesystem::path A(A_str),B(B_str);
    auto rel = B.lexically_relative(A);
    if (rel.empty())
        return 1;
    auto it = rel.begin();
    if (it != rel.end() && *it == "..")
        return 0;
    return 1;
}
long long int str_to_size(const std::string& input)//This function was helped by DeepSeek
{
    std::istringstream iss(input);
    double value;
    iss >> value;
    if (iss.fail())
        throw std::runtime_error("Invalid size.\n");
    char unit = 0;
    iss >> unit;
    if (iss.fail())
    {
        if (!iss.eof())
            throw std::runtime_error("Invalid size.\n");
        if (value < 0)
            throw std::runtime_error("Invalid size.\n");
        return static_cast<long long>(std::llround(value));
    }
    std::string rest;
    iss >> rest;
    if (!rest.empty())
        throw std::runtime_error("Invalid size.\n");

    long long multiplier;
    switch (std::toupper(static_cast<unsigned char>(unit)))
    {
    case 'K':
        multiplier = 1024LL; break;
    case 'M':
        multiplier = 1024LL * 1024; break;
    case 'G':
        multiplier = 1024LL * 1024 * 1024; break;
    case 'T':
        multiplier = 1024LL * 1024 * 1024 * 1024; break;
    default:
        throw std::runtime_error("Invalid size.\n");
    }
    if (value < 0)
        throw std::runtime_error("Invalid size.\n");
    return static_cast<long long>(std::llround(value * multiplier));
}
long long int get_file_size(const std::string& file_path)
{
    try
    {
        std::uintmax_t size = std::filesystem::file_size(file_path);
        return static_cast<long long int>(size);
    }
    catch(const std::filesystem::filesystem_error &e)
    {
        throw std::runtime_error("Failed to get the size of "+file_path+": "+e.what());
    }
}
struct file_list
{
    std::vector<sum_and_dir> file_index;
    std::vector<std::string> error_stack;
    bool log=1;
    std::string logtag;
    //File list operations
    file_list break_apart(const int& amount,int offset)
    {
        file_list res;
        if(cmd::find_key("--contiguous"))
            res.file_index.insert(res.file_index.end(),file_index.begin()+file_index.size()*offset/amount,file_index.begin()+(offset==amount-1?file_index.size():file_index.size()*(offset+1)/amount));
        else
        {
            offset=offset%amount,res.log=log;
            for(int i=offset;i<file_index.size();i+=amount)
                res.file_index.push_back(file_index[i]);
        }
        return res;
    }
    void clear_invalid_sum()
    {
        std::vector<sum_and_dir> new_flist;
        bool invalid;
        for(int i=0;i<file_index.size();i++)
        {
            invalid=1;
            for(int j=0;j<file_index[i].sum.length();j++)
                if(!std::isxdigit(file_index[i].sum[j]))
                {
                    invalid=0,error_stack.push_back("Invalid sum for "+file_index[i].dir+", ignored.");
                    break;
                }
            if(invalid)
                new_flist.push_back(file_index[i]);
        }
        file_index=new_flist;
    }
    void clear_marked()
    {
        std::vector<sum_and_dir> new_flist;
        for(int i=0;i<file_index.size();i++)
            if(!file_index[i].mark)
                new_flist.push_back(file_index[i]);
        file_index=new_flist;
    }
    void clear_masked()
    {
        std::vector<sum_and_dir> new_flist;
        for(int i=0;i<file_index.size();i++)
            if(!file_index[i].mask)
                new_flist.push_back(file_index[i]);
        file_index=new_flist;
    }
    void clear_dup()
    {
        if(file_index.empty())
            return;
        std::sort(file_index.begin(),file_index.end(),cmp::cmp_by_dir);
        std::vector<sum_and_dir> new_flist;
        for(int i=1;i<file_index.size();i++)
            if(file_index[i].dir==file_index[i-1].dir)
                file_index[i].mark=file_index[i-1].mark+1;
        for(int i=0;i<file_index.size();i++)
            if(!file_index[i].mark)
                new_flist.push_back(file_index[i]);
        file_index=new_flist;
    }
    void merge_with(const file_list& target)//ignoring sum and type
    {
        file_index.insert(file_index.end(),target.file_index.begin(),target.file_index.end());
        error_stack.insert(error_stack.end(),target.error_stack.begin(),target.error_stack.end());
        clear_dup();
    }
    //Loading
    void load_files_from_dir(const std::string& dir_path)//This function was helped by DeepSeek
    {
        if(log)
            std::clog<<"-->"<<logtag<<"Searching for regular files in "<<dir_path<<" ...\n";
        sum_and_dir tmp;
        tmp.sum="<NONE>";
        std::filesystem::path base_dir = dir_path;
        if(!std::filesystem::exists(base_dir))
        {
            std::cerr << "==>"<<logtag<<"Error: Directory '" << dir_path << "' does not exist." << std::endl;
            return;
        }
        bool allow_symlinks = cmd::find_key("--allow-symlinks");
        auto options = std::filesystem::directory_options::skip_permission_denied;
        if (allow_symlinks)
            options |= std::filesystem::directory_options::follow_directory_symlink;
        try
        {
            for(const auto &entry : std::filesystem::recursive_directory_iterator(base_dir, options))
            {
                if(std::filesystem::is_regular_file(entry.status()) ||(allow_symlinks && std::filesystem::is_symlink(entry.status())))
                {
                    std::filesystem::path rel_path = std::filesystem::relative(entry.path(), base_dir);
                    rel_path = base_dir / rel_path,tmp.dir = rel_path.string();
                    file_index.push_back(tmp);
                }
            }
        }
        catch (const std::filesystem::filesystem_error& msg)
        {
            throw std::runtime_error("Fatal: Filesystem error while traversing "+dir_path+": "+msg.what());
        }
        clear_dup();
        if(log)
            std::clog<<"-->"<<logtag<<"Found "<<file_index.size()<<" regular files in "<<dir_path<<" .\n";
    }
    void load_files_from_stream(const bool& is_classed,const std::string& class_to_add,std::istream& input,const std::string& source)
    {
        if(log)
            std::clog<<"-->"<<logtag<<"Loading file list from stdin ...\n";
        std::string tmp;
        sum_and_dir tmp_data;
        int i=0;
        while(std::getline(input,tmp))
        {
            if(tmp.empty()||tmp[0]=='#')
                continue;
            tmp_data.dir="",tmp_data.sum="",tmp_data.type=class_to_add;
            try
            {
                tmp_data.import(tmp,is_classed,++i,source);
            }
            catch(const std::exception& msg)
            {
                error_stack.push_back(msg.what());
                continue;
            }
            file_index.push_back(tmp_data);
        }
        clear_dup();
    }
    void load_plain_flist_from_stream(std::istream& input)
    {
        if(log)
            std::clog<<"-->"<<logtag<<"Loading file list from stdin ...\n";
        sum_and_dir tmp_data;
        tmp_data.sum="<NONE>";
        while(std::getline(input,tmp_data.dir))
        {
            if(tmp_data.dir.empty()||tmp_data.dir[0]=='#')
                continue;
            file_index.push_back(tmp_data);
        }
        clear_dup();
    }
    //Filtering
    void filter_path_by_dir(const std::string& target,const bool& mode)//mode==1-->focus,mode==0->exclude
    {
        std::vector<sum_and_dir> new_flist;
        for(int i=0;i<file_index.size();i++)
        {
            if(B_is_As_subpath(target,file_index[i].dir)^mode)
                continue;
            new_flist.push_back(file_index[i]);
        }
        file_index=new_flist;
    }
    void filter_path_by_size_limit(const long long int& size_limit,const bool& mode)//mode==0-->max,mode==1-->min
    {
        std::vector<sum_and_dir> new_flist;
        for(int i=0;i<file_index.size();i++)
        {
            if((file_index[i].size>size_limit&&!mode)||(file_index[i].size<size_limit&&mode))
                continue;
            new_flist.push_back(file_index[i]);
        }
        file_index=new_flist;
    }
    void filter_by_args()
    {
        clear_masked();
        std::vector<std::string> excludings=cmd::get_arr("--exclude"),focusings=cmd::get_arr("--focus");
        int tmpsize;
        for(int i=0;i<excludings.size();i++)//--exclude
        {
            tmpsize=file_index.size();
            filter_path_by_dir(excludings[i],0);
            if(log)
                std::clog<<"-->"<<logtag<<"Ignored "<<tmpsize-file_index.size()<<" files for argument: --exclude="<<excludings[i]<<".\n";
        }
        if(focusings.size()!=0)
        {
            std::clog<<"-->"<<logtag<<"Recalculating file amount for option: --focus...\n";//--focus
            file_list res,tmp;
            for(int i=0;i<focusings.size();i++)
            {
                tmpsize=res.file_index.size();
                tmp.file_index=file_index,tmp.log=log;
                tmp.filter_path_by_dir(focusings[i],1);
                res.merge_with(tmp);
                if(log)
                    std::clog<<"-->"<<logtag<<"Included "<<res.file_index.size()-tmpsize<<" files for argument: --focus="<<focusings[i]<<".\n";
            }
            file_index=res.file_index;
        }
        if(cmd::find_key("--max-size"))//--max-size
        {
            tmpsize=file_index.size();
            long long int max_size=str_to_size(cmd::get_arr("--max-size")[0]);
            filter_path_by_size_limit(max_size,0);
            if(log)
                std::clog<<"-->"<<logtag<<"Ignored "<<tmpsize-file_index.size()<<" files for argument: --max-size="<<cmd::get_arr("--max-size")[0]<<" ("<<max_size<<" Bytes).\n";
        }
        if(cmd::find_key("--min-size"))//--min-size
        {
            tmpsize=file_index.size();
            long long int min_size=str_to_size(cmd::get_arr("--min-size")[0]);
            filter_path_by_size_limit(min_size,1);
            if(log)
                std::clog<<"-->"<<logtag<<"Ignored "<<tmpsize-file_index.size()<<" files for argument: --min-size="<<cmd::get_arr("--min-size")[0]<<" ("<<min_size<<" Bytes).\n";
        }
    }
    //Expressions
    void print_list_to_stream(const bool& is_classed,const bool& show_sum,std::ostream& output)
    {
        for(int i=0;i<file_index.size();i++)
            output<<(is_classed?file_index[i].type+" ":"")<<(show_sum?file_index[i].sum+" ":"")<<file_index[i].dir<<std::endl;
    }
    void print_err_to_stream(const std::string& head,std::ostream& output)
    {
        for(int i=0;i<error_stack.size();i++)
            output<<head<<": "<<error_stack[i]<<std::endl;
    }
    //Calculations
    void refresh_file_size()
    {
        for(int i=0;i<file_index.size();i++)
            try
            {
                file_index[i].size=get_file_size(file_index[i].dir);
            }
            catch(const std::exception& msg)
            {
                file_index[i].mask=1,file_index[i].size=0,error_stack.push_back(msg.what());
            }
    }
    long long int total_size(const bool& keep)
    {
        long long int res=0;
        for(int i=0;i<file_index.size();i++)
        {
            if((file_index[i].sum!="<NONE>"&&keep)||file_index[i].mask)
                continue;
            res+=file_index[i].size;
        }
        return res;
    }
};
}
namespace sum
{
std::atomic<bool> kill_flag{0};
std::atomic<int> processed{0},kept{0};
std::string compress_string(std::string target)
{
    if (target.length() > 100)
    {
        target.resize(97);
        target.append("...");
    }
    else
        target.resize(100, ' ');
    return target;
}
long long int calculate_buffer_size(const bool& dynamic_bsize,const long long int& size_limit,const long long int& target_size)
{
    if(!dynamic_bsize)
        return size_limit;
    else
    {
        if(target_size<16LL*1024)//<16KB-->2KB
            return std::min(size_limit,2LL*1024);
        else if(target_size<256LL*1024)//<256KB-->4KB
            return std::min(size_limit,4LL*1024);
        else if(target_size<4096LL*1024)//<4096KB-->8KB
            return std::min(size_limit,8LL*1024);
        else if(target_size<65536LL*1024)//<=65536KB-->16KB
            return std::min(size_limit,16LL*1024);
        else//>=65536KB-->64KB
            return std::min(size_limit,64LL*1024);
    }
}
template<typename Context, int DigestLength, int (*InitFunc)(Context*), int (*UpdateFunc)(Context*, const void*, size_t), int (*FinalFunc)(unsigned char*, Context*)>
std::string compute_hash(std::ifstream &file, const std::string &file_path,const bool& dynamic_bsize, const long long int& size_limit,const long long int& target_size)
{
    Context context;
    if(!InitFunc(&context))
        throw std::runtime_error("Hash initialization failed while calculating " + file_path);
    size_t buf_size = calculate_buffer_size(dynamic_bsize, size_limit, target_size);
    std::vector<char> buffer(buf_size);
    while(file.read(buffer.data(), buffer.size()) || file.gcount() > 0)
        if(!UpdateFunc(&context, buffer.data(), file.gcount()))
            throw std::runtime_error( "Hash update failed while calculating " + file_path);
    unsigned char hash[DigestLength];
    if(!FinalFunc(hash, &context))
        throw std::runtime_error( "Hash final calculation failed while calculating " + file_path);
    std::stringstream ss;
    for(int i = 0; i < DigestLength; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    return ss.str();
}
std::string calculate_file_checksum(const std::string &file_path, const std::string &checksum_type,const bool& dynamic_bsize,const long long int& size_limit,const long long int& target_size)
{
    std::ifstream file(file_path, std::ios::binary);
    if(!file.good())
        throw std::runtime_error( "Failed to open file: " + file_path);
    if(checksum_type == "md5")
        return compute_hash<MD5_CTX, MD5_DIGEST_LENGTH, MD5_Init, MD5_Update, MD5_Final>(file, file_path,dynamic_bsize,size_limit,target_size);
    else if(checksum_type == "sha1")
        return compute_hash<SHA_CTX, SHA_DIGEST_LENGTH, SHA1_Init, SHA1_Update, SHA1_Final>(file, file_path,dynamic_bsize,size_limit,target_size);
    else if(checksum_type == "sha256")
        return compute_hash<SHA256_CTX, SHA256_DIGEST_LENGTH, SHA256_Init, SHA256_Update, SHA256_Final>(file, file_path,dynamic_bsize,size_limit,target_size);
    else if(checksum_type == "sha512")
        return compute_hash<SHA512_CTX, SHA512_DIGEST_LENGTH, SHA512_Init, SHA512_Update, SHA512_Final>(file, file_path,dynamic_bsize,size_limit,target_size);
    else if(checksum_type == "none")
        return "<NONE>";
    else
        throw std::runtime_error( "Unsupported checksum type: " + checksum_type);
}
bool is_sum(std::string x)
{
    if(x == "sha256" || x == "sha1" || x == "md5" || x == "sha512"||x=="none")
        return 1;
    return 0;
}
int calculate_dir_checksum(file::file_list &data, const std::string sum_type,const int in_detail,const bool& dynamic_bsize,const long long int& size_limit,const bool& keep,const int& ID)
{
    if(!is_sum(sum_type))
    {
        std::cerr << "==>SUM: Error: '" << sum_type << "' is not a supported checksum type." << std::endl;
        return 1;
    }
    std::string tmpsum;
    int total=data.file_index.size();
    for(int i = 0; i < total; i++)
    {
        if(kill_flag.load()==1)
        {
            std::fprintf(stderr,"==>SUM: Thread #%d: Received termination signal, exiting...\n",ID);
            processed+=i;
            return 1;
        }
        if(data.file_index[i].mask)
        {
            if(data.file_index[i].sum.empty())
                data.file_index[i].sum="<NONE>";
            data.error_stack.push_back("Cannot process "+data.file_index[i].dir+": forbidden by context.");
        }
        if(data.file_index[i].sum!="<NONE>"&&keep)
        {
            kept++;
            continue;
        }
        if(in_detail != 0)
        {
            std::clog << '[' << i << '/' << total << "][";
            if(in_detail==1)
            {
                int j = 0;
                for(; j < 100; j++)
                    if(j <= (i) * 100 / total)
                        std::clog << '#';
                    else
                        break;
                for(; j < 100; j++)
                    std::clog << '.';
            }
            else
                std::clog<<compress_string(data.file_index[i].dir);
            std::clog << "]\r" << std::flush;
        }
        if(sum_type!="none")
            try
            {
                tmpsum=calculate_file_checksum(data.file_index[i].dir,sum_type,dynamic_bsize,size_limit,data.file_index[i].size);
            }
            catch(const std::exception& msg)
            {
                tmpsum="<NONE>";
                data.error_stack.push_back(msg.what());
            }
        else if(data.file_index[i].sum.empty())
            tmpsum="<NONE>";
        else
            continue;
        data.file_index[i].sum=tmpsum;
    }
    processed+=data.file_index.size();
    return 0;
}
}
namespace major_func
{
namespace lib
{
void signal_handler(int signal_flag)
{
    if(signal_flag==SIGINT)
        sum::kill_flag=1;
}
std::string current_time()
{
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time = *std::localtime(&time);
    std::stringstream ss;
    ss << std::put_time(&local_time, "%Y-%m-%d-%H-%M-%S");
    return ss.str();
}
}
int mode_sum(int argc, char** argv)
{
    if(argc <= 3)//Check cmdline
    {
        std::cerr << "==>SUM: Error: At least three arguments are required for 'sum' mode." << std::endl;
        return 1;
    }
    if(!sum::is_sum(std::string(argv[2])))
    {
        std::cerr<<"==>SUM: Error: "<<std::string(argv[2])<<" is not a supported checksum type.\n";
        return 1;
    }
    std::filesystem::path source = std::string(argv[3]);
    int load_from_stdin=0;
    bool allow_log=!cmd::find_key("--quiet");
    if(source.string()=="stdin")//stdin
    {
        if(allow_log)
            std::clog<<"\n-->SUM: Will load file list (tsubaki file list format) from stdin...\n";
        load_from_stdin=1;
    }
    else if(source.string()=="stdin-plain-list")
    {
        if(allow_log)
            std::clog<<"\n-->SUM: Will load file list (plain list format) from stdin...\n";
        load_from_stdin=2;
    }
    else if(std::filesystem::is_regular_file(source))//Regular file
    {
        if(allow_log)
            std::clog <<std::endl<<"-->SUM: "<< std::string(argv[3]) << " is a regular file." << std::endl << "Calculating " << std::string(argv[2]) << " checksum of this file..." << std::endl;
        try
        {
            std::cout << sum::calculate_file_checksum(std::string(argv[3]),std::string(argv[2]),0,16LL*1024,0)<<' '<<std::string(argv[3])<< std::endl;
        }
        catch(const std::exception& msg)
        {
            std::cerr<<"==>SUM: Error: "<<msg.what()<<std::endl;
            return 1;
        }
        return 0;
    }
    else if(!std::filesystem::is_directory(source))//Wrong type
    {
        std::cerr << "==>SUM: Error: " << std::string(argv[3]) << " is not a regular file or a directory." << std::endl;
        return 1;
    }
    else if(allow_log)
        std::clog<<"\n-->SUM: Will load file list from "<<source.string()<<" ...\n";
    file::file_list data;//Major data structure
    data.log=allow_log;
    data.logtag="SUM: ";
    if(!load_from_stdin)//Load the file-list
        data.load_files_from_dir(source.string());
    else if(load_from_stdin==1)
        data.load_files_from_stream(0,"",std::cin,"stdin");
    else
        data.load_plain_flist_from_stream(std::cin);
    data.refresh_file_size();//Calculate total file size
    data.filter_by_args();//Filter the file-list
    if(allow_log)
        std::clog<<"-->SUM: Eventually "<<data.file_index.size()<<" regular files in "<<source.string()<<" were loaded in memory.\n";
    bool keep=!cmd::find_key("--force-rescan");
    long long int total_size=data.total_size(keep);
    bool dynamic_bsize=1;//Calculate buffer size
    if(cmd::find_key("--buffer-size"))
        dynamic_bsize=0;
    long long int bsize_limit=(dynamic_bsize?(cmd::find_key("--max-buffer-size")?file::str_to_size(cmd::get_arr("--max-buffer-size")[0]):64LL*1024):file::str_to_size(cmd::get_arr("--buffer-size")[0]));
    if(bsize_limit<1LL*1024||bsize_limit>64LL*1024*1024)
    {
        std::cerr<<"==>SUM: Error: Buffer size is too large or too small.\n";
        return 1;
    }
    if(allow_log)
        std::clog<<"\n-->SUM: Using "<<(dynamic_bsize?"dynamic buffer size (<=":"static buffer size (=")<<(dynamic_bsize?std::min(64LL*1024,bsize_limit):bsize_limit)<<" bytes).\n";
    int allow_multi_thd=1;//Calc the amount of thd
    if(std::string(argv[2])=="none"||total_size<=1LL*1024*1024*1024||cmd::find_key("--disable-multi-thd"))
        allow_multi_thd=0;
    int thd_amount=cmd::get_thd_amount();
    if(thd_amount<=0)
        thd_amount=4;
    if(!allow_multi_thd)
        thd_amount=1;
    if(thd_amount==1)
        allow_multi_thd=0;
    if(allow_log)
    {
        std::clog<<"\n-->SUM: Total size is "<<total_size<<" bytes, "<<(total_size/1024)/1024<<" MB or "<<((total_size/1024)/1024)/1024<<" GB.\n";
        std::clog<<"==>SUM: Attention: All results will be printed to stdout. Redirect stdout to a text file if you want to save the results.\n";
        if(!allow_multi_thd)
            std::clog<<"-->SUM: Notice: Multi-thread is disabled.\n";
        else
            std::clog<<"-->SUM: Notice: Multi-thread is enabled. \n                "
                      <<thd_amount<<" threads will be started.\n                Using "
                      <<(cmd::find_key("--contiguous")?"contiguous ":"interleaved ")<<"chunking.\n";
        std::clog<<"\n-->SUM: Calculating checksums..." << std::endl;
    }
    if(cmd::find_key("--test"))//Pause
    {
        std::clog<<"-->SUM: Calculation will not be started for argument: --test.\n";
        return 0;
    }
    auto start_time=std::chrono::steady_clock::now();//Start the timer
    std::vector<file::file_list> sub_data;
    std::vector<std::thread> calc;
    int monitor_thd=0;
    long long int max_subdata_size=0,this_size;
    for(int i=0;i<thd_amount;i++)//Chunk
    {
        file::file_list tmp_list=data.break_apart(thd_amount,i);
        sub_data.push_back(tmp_list);
        this_size=sub_data[i].total_size(keep);
        if(max_subdata_size<this_size)
            monitor_thd=i,max_subdata_size=this_size;
    }
    data.file_index.clear();
    if(cmd::verbose())
        std::clog<<"==>SUM: Attention: This progress only stands for one of the threads (Thread #"<<monitor_thd<<"), because this thread is assigned the largest total file size.\n";
    signal(SIGINT,lib::signal_handler);//Start the major calc progress
    for(int i=0;i<thd_amount;i++)
        calc.emplace_back(sum::calculate_dir_checksum,std::ref(sub_data[i]),std::string(argv[2]),(i==monitor_thd?cmd::verbose():0),dynamic_bsize,bsize_limit,keep,i);
    for(int i=0;i<thd_amount;i++)
        calc[i].join();
    for(int i=0;i<thd_amount;i++)
        data.file_index.insert(data.file_index.end(),sub_data[i].file_index.begin(),sub_data[i].file_index.end()),
            data.error_stack.insert(data.error_stack.end(),sub_data[i].error_stack.begin(),sub_data[i].error_stack.end());
    auto end_time=std::chrono::steady_clock::now();//Stop the timer
    auto duration=std::chrono::duration_cast<std::chrono::microseconds>(end_time-start_time);
    if(allow_log)//Sum up
    {
        std::clog << "                                                                                                                        \r" << std::flush;
        std::clog << "-->SUM: Calculation finished in "<<duration.count()/1000000.0<<" secs." << std::endl;
    }
    bool plain_list=(std::string(argv[2])=="none")&&cmd::find_key("--plain-list");
    data.print_list_to_stream(0,!plain_list,std::cout);
    if(plain_list)
        return 0;
    std::cout<<"#\n#----------General Report----------"<<
        "\n#Total:          "<<data.file_index.size()<<
        "\n#Succeed:        "<<sum::processed.load()-data.error_stack.size()-sum::kept.load()<<
        "\n#Failed:         "<<data.error_stack.size()<<
        "\n#Kept:           "<<sum::kept.load()<<
        "\n#Unprocessed:    "<<data.file_index.size()-sum::processed.load()<<
        "\n#Time Finished:  "<<lib::current_time()<<
        "\n#Duration:       "<<duration.count()/1000000.0<<" secs"<<
        "\n#Command:        ";
    for(int i=0;i<argc;i++)
        std::cout<<std::string(argv[i])<<' ';
    if(data.error_stack.size() != 0)
    {
        std::cout << "\n#Error messages:\n";
        data.print_err_to_stream("#Error",std::cout);
    }
    if(!data.error_stack.empty())
    {
        std::cerr << "==>SUM: Several errors were reported. The collected error messages have been appended to the end of stdout." << std::endl;
        return 1;
    }
    return 0;
}
int mode_mrg(int argc, char** argv)
{
    if(argc <= 3)
    {
        std::cerr << "==>MRG: Error: At least three arguments are required for 'mrg' mode." << std::endl;
        return 1;
    }
    std::string inputA(argv[2]), inputB(argv[3]), tmp;
    if(!std::filesystem::exists(inputA))
    {
        std::cerr << "==>MRG: Error: Doesn't exist: " << inputA << std::endl;
        return 1;
    }
    if(!std::filesystem::exists(inputB))
    {
        std::cerr << "==>MRG: Error: Doesn't exist: " << inputB << std::endl;
        return 1;
    }
    std::ifstream streamA(inputA), streamB(inputB);
    if(!streamA.good())
    {
        std::cerr << "==>MRG: Error: Cannot read the file: " << inputA << std::endl;
        return 1;
    }
    if(!streamB.good())
    {
        std::cerr << "==>MRG: Error: Cannot read the file: " << inputB << std::endl;
        return 1;
    }
    file::file_list data;
    data.log=!cmd::find_key("--quiet");
    data.logtag="MRG: ";
    data.load_files_from_stream(0,"A",streamA,inputA);
    data.load_files_from_stream(0,"B",streamB,inputB);
    data.print_list_to_stream(1,1,std::cout);
    if(data.error_stack.size()!=0)
    {
        std::cout << "\n#Error messages:\n";
        data.print_err_to_stream("#Error",std::cout);
    }
    if(!cmd::find_key("--quiet"))
        std::clog << "-->MRG: Merging finished." << std::endl;
    if(!data.error_stack.empty())
    {
        std::cerr << "==>MRG: Several errors were reported. The collected error messages have been appended to the end of stdout." << std::endl;
        return 1;
    }
    return 0;
}
int mode_dup()
{
    bool log=!cmd::find_key("--quiet");
    file::file_list data;
    data.log=log;
    data.logtag="DUP: ";
    data.load_files_from_stream(0,"",std::cin,"stdin");
    data.clear_invalid_sum();
    if(log)
        std::clog << "-->DUP: Calculating duplication..." << std::endl;
    data.clear_dup();
    std::sort(data.file_index.begin(),data.file_index.end(),cmp::cmp_by_sum);
    if(log)
        std::clog << "-->DUP: Duplication calculation finished, collecting and printing data to stdout..." << std::endl;
    for(int i=1;i<data.file_index.size();i++)
        if(data.file_index[i].sum==data.file_index[i-1].sum)
            data.file_index[i].mark=data.file_index[i-1].mark+1;
    int dup_cnt=0;
    std::vector<std::string> recommend;
    for(int i=1;i<data.file_index.size();i++)
        if(data.file_index[i].mark)
        {
            std::cout<<'['<<++dup_cnt<<"] Checksum: "<<data.file_index[i].sum<<std::endl;
            std::cout<<'['<<dup_cnt<<"] "<<data.file_index[i-1].dir<<std::endl;
            for(;data.file_index[i].mark;i++)
                recommend.push_back(data.file_index[i].dir),std::cout<<'['<<dup_cnt<<"] "<<data.file_index[i].dir<<std::endl;
            std::cout<<std::endl;
            i--;
        }
    if(dup_cnt)
        std::cout<<"\nRecommended clearing command: rm ";
    for(int i=0;i<recommend.size();i++)
        std::cout<<"\'"<<recommend[i]<<"\' ";
    if(data.error_stack.size() != 0)
    {
        std::cout << "\n\n[E] Error messages:\n";
        data.print_err_to_stream("[E] Error",std::cout);
    }
    if(log)
        std::clog << "-->DUP: Duplication calculation finished." << std::endl;
    if(!data.error_stack.empty())
    {
        std::cerr << "==>DUP: Several errors were reported. The collected error messages have been appended to the end of stdout." << std::endl;
        return 1;
    }
    return 0;
}
int mode_cmp()
{
    bool log=!cmd::find_key("--quiet");
    file::file_list data;
    data.log=log;
    data.logtag="CMP: ";
    data.load_files_from_stream(1,"",std::cin,"stdin");
    data.clear_invalid_sum();
    std::vector<std::string> modified, matched;
    sum_and_dir tmpdata;
    std::string tmp;
    int i = 0;
    if(log)
        std::clog << "-->CMP: Calculating phase 1 comparison..." << std::endl;
    std::sort(data.file_index.begin(), data.file_index.end(), cmp::cmp_by_dir);
    if(log)
        std::clog << "-->CMP: Phase 1 comparison finished, collecting and printing data to stdout..." << std::endl;
    for(i = 1; i < data.file_index.size(); i++)
        if(data.file_index[i].dir == data.file_index[i - 1].dir)
        {
            if(data.file_index[i].sum == data.file_index[i - 1].sum)
                data.file_index[i].mark = 1, data.file_index[i - 1].mark = 1,matched.push_back(data.file_index[i].dir);
            else
                data.file_index[i].mark = 1, data.file_index[i - 1].mark = 1,modified.push_back(data.file_index[i].dir);
        }
    std::cout << "[!] Modified:" << std::endl;
    for(i = 0; i < modified.size(); i++)
        std::cout<<"[!] " << modified[i] << std::endl;
    std::cout << std::endl << "[D] Moved,copied,merged or renamed:" << std::endl;
    if(log)
        std::clog << "-->CMP: Calculating phase 2 comparison..." << std::endl;
    data.clear_marked();
    std::sort(data.file_index.begin(), data.file_index.end(), cmp::cmp_by_sum);
    if(log)
        std::clog << "-->CMP: Phase 2 comparison finished, collecting and printing data to stdout..." << std::endl;
    std::vector<std::string> A_tmpdir, B_tmpdir, A_del_or_add, B_del_or_add;
    int cnt = 0;
    for(i = 0; i < data.file_index.size(); i++)
    {
        if(data.file_index[i].type == "A")
            A_tmpdir.push_back(data.file_index[i].dir);
        else
            B_tmpdir.push_back(data.file_index[i].dir);
        if(i == data.file_index.size() - 1 || data.file_index[i].sum != data.file_index[i + 1].sum)
        {
            if(A_tmpdir.size() == 0)
                B_del_or_add.insert(B_del_or_add.end(), B_tmpdir.begin(), B_tmpdir.end());
            else if(B_tmpdir.size() == 0)
                A_del_or_add.insert(A_del_or_add.end(), A_tmpdir.begin(), A_tmpdir.end());
            else
            {
                std::cout << "[D][" << ++cnt << "]" << std::endl << "[D][" << cnt << "][A]\n";
                for(int j = 0; j < A_tmpdir.size(); j++)
                    std::cout << "[D]["<<cnt<<"][A]" << A_tmpdir[j] << std::endl;
                std::cout << "[D][" << cnt << "][B]\n";
                for(int j = 0; j < B_tmpdir.size(); j++)
                    std::cout << "[D]["<<cnt<<"][B]" << B_tmpdir[j] << std::endl;
            }
            A_tmpdir.clear();
            B_tmpdir.clear();
        }
    }
    std::cout << std::endl << "[U] Deleted or added:" << std::endl << "[U][A]\n";
    for(i = 0; i < A_del_or_add.size(); i++)
        std::cout <<"[U][A]"<< A_del_or_add[i] << std::endl;
    std::cout<< "[U][B]\n";
    for(i = 0; i < B_del_or_add.size(); i++)
        std::cout <<"[U][B]"<< B_del_or_add[i] << std::endl;
    std::cout << std::endl<< "[=] Matched:" << std::endl;
    for(i = 0; i < matched.size(); i++)
        std::cout <<"[=] "<< matched[i] << std::endl;
    if(data.error_stack.size() != 0)
    {
        std::cout << "\n[E] Error messages:\n";
        data.print_err_to_stream("[E] Error",std::cout);
    }
    if(log)
        std::clog << "-->CMP: Comparison finished." << std::endl;
    if(!data.error_stack.empty())
    {
        std::cerr << "==>CMP: Several errors were reported. The collected error messages have been appended to the end of stdout." << std::endl;
        return 1;
    }
    return 0;
}
int mode_help()
{
    std::cout<<R"(
Tsubaki – checksum utility for file integrity, comparison & duplicate detection

USAGE: tsubaki <command> [options]

COMMANDS:
  sum <type> <path> [options]   Compute checksums for files/directories.
    <type>: md5|sha1|sha256|sha512|none
    <path>: file, directory, "stdin" (tsubaki format) or "stdin-plain-list"
    Options:
      --exclude=<dir>           Exclude files under <dir> (can repeat)
      --focus=<dir>             Only include files under <dir> (union)
      --max-size=<size>         Skip files larger than <size> (e.g. 10M, 2G)
      --min-size=<size>         Skip files smaller than <size>
      --allow-symlinks          Follow symlinks
      --test                    Dry run (no checksum calculation)
      --quiet                   Suppress informational messages
      --plain-list              With type=none: output only paths
      --disable-multi-thd       Disable multi‑threading
      --thd-amount=<N>          Set number of threads
      --contiguous              Chunk file list contiguously for threads
      --force-rescan            Recompute checksums even if present (stdin)
      --buffer-size=<size>      Static buffer size (default dynamic ≤64K)
      --max-buffer-size=<size>  Upper bound for dynamic buffer
      -v, -vv                   Verbosity (progress bar / file names)

  mrg <fileA> <fileB> [options] Merge two checksum lists (adds 'A '/'B ' prefix).
    Options: --quiet

  cmp [options]                 Compare A/B‑prefixed lists from stdin.
    Groups: [!] Modified, [D] Moved/copied, [U] Deleted/added, [=] Matched.
    Options: --quiet

  dup [options]                 Find duplicates from stdin list (<sum> <path>).
    Options: --quiet

  help                          Show this help.

SIZE FORMAT: optional suffix K (KiB), M (MiB), G (GiB), T (TiB); fractions allowed.

EXIT STATUS: 0 success, 1 error.

EXAMPLES:
  tsubaki sum sha256 /home/user --exclude=.cache > sums.txt
  cat paths.txt | tsubaki sum md5 stdin-plain-list
  tsubaki sum none /dir --plain-list > file-list.txt
  tsubaki mrg A.txt B.txt | tsubaki cmp > comparison.txt
  tsubaki sum md5 /photos | tsubaki dup

For full documentation, see README or the original help message.
)";
    return 0;
}
}
int main(int argc, char** argv)
{
    try
    {
        if(argc == 1)
        {
            std::cerr << "==>Error: At least one argument is required.\nType 'tsubaki help' for usage." << std::endl;
            return 1;
        }
        std::string mode(argv[1]);
        cmd::init(argc,argv);
        if(mode == "sum")
            return major_func::mode_sum(argc, argv);
        else if(mode == "mrg")
            return major_func::mode_mrg(argc, argv);
        else if(mode == "cmp")
            return major_func::mode_cmp();
        else if(mode == "help")
            return major_func::mode_help();
        else if(mode=="dup")
            return major_func::mode_dup();
        else
        {
            std::cerr << "==>Error: Unknown subcommand.\nType 'tsubaki help' for usage." << std::endl;
            return 1;
        }
    }
    catch(std::exception& msg)
    {
        std::cerr<<"==>Error caught in main(): \n"<<msg.what()<<std::endl;
        return 1;
    }
    return 0;
}
