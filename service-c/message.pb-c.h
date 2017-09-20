/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: message.proto */

#ifndef PROTOBUF_C_message_2eproto__INCLUDED
#define PROTOBUF_C_message_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1000000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1002001 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _HeroMsg HeroMsg;
typedef struct _EnemyMsg EnemyMsg;
typedef struct _NewEnemy NewEnemy;
typedef struct _EnemyLeave EnemyLeave;
typedef struct _LoginReq LoginReq;
typedef struct _LoginRsp LoginRsp;
typedef struct _StartReq StartReq;
typedef struct _StartRsp StartRsp;
typedef struct _LoginEnd LoginEnd;
typedef struct _LeaveReq LeaveReq;
typedef struct _LeaveRsp LeaveRsp;
typedef struct _MoveReq MoveReq;
typedef struct _MoveRsp MoveRsp;


/* --- enums --- */


/* --- messages --- */

struct  _HeroMsg
{
  ProtobufCMessage base;
  int32_t uid;
  int32_t point_x;
  int32_t point_y;
};
#define HERO_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&hero_msg__descriptor) \
    , 0, 0, 0 }


struct  _EnemyMsg
{
  ProtobufCMessage base;
  int32_t uid;
  int32_t point_x;
  int32_t point_y;
};
#define ENEMY_MSG__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&enemy_msg__descriptor) \
    , 0, 0, 0 }


struct  _NewEnemy
{
  ProtobufCMessage base;
  int32_t uid;
  int32_t point_x;
  int32_t point_y;
};
#define NEW_ENEMY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&new_enemy__descriptor) \
    , 0, 0, 0 }


struct  _EnemyLeave
{
  ProtobufCMessage base;
  int32_t uid;
};
#define ENEMY_LEAVE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&enemy_leave__descriptor) \
    , 0 }


struct  _LoginReq
{
  ProtobufCMessage base;
  char *name;
};
#define LOGIN_REQ__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&login_req__descriptor) \
    , NULL }


struct  _LoginRsp
{
  ProtobufCMessage base;
  protobuf_c_boolean success;
  int32_t uid;
  int32_t point_x;
  int32_t point_y;
  int32_t enemy_num;
};
#define LOGIN_RSP__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&login_rsp__descriptor) \
    , 0, 0, 0, 0, 0 }


struct  _StartReq
{
  ProtobufCMessage base;
  protobuf_c_boolean start;
};
#define START_REQ__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&start_req__descriptor) \
    , 0 }


struct  _StartRsp
{
  ProtobufCMessage base;
  protobuf_c_boolean start;
};
#define START_RSP__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&start_rsp__descriptor) \
    , 0 }


struct  _LoginEnd
{
  ProtobufCMessage base;
  protobuf_c_boolean success;
};
#define LOGIN_END__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&login_end__descriptor) \
    , 0 }


struct  _LeaveReq
{
  ProtobufCMessage base;
  int32_t uid;
};
#define LEAVE_REQ__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&leave_req__descriptor) \
    , 0 }


struct  _LeaveRsp
{
  ProtobufCMessage base;
  protobuf_c_boolean leave;
};
#define LEAVE_RSP__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&leave_rsp__descriptor) \
    , 0 }


struct  _MoveReq
{
  ProtobufCMessage base;
  int32_t move;
};
#define MOVE_REQ__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&move_req__descriptor) \
    , 0 }


struct  _MoveRsp
{
  ProtobufCMessage base;
  protobuf_c_boolean success;
  int32_t uid;
  int32_t pos_x;
  int32_t pos_y;
};
#define MOVE_RSP__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&move_rsp__descriptor) \
    , 0, 0, 0, 0 }


/* HeroMsg methods */
void   hero_msg__init
                     (HeroMsg         *message);
size_t hero_msg__get_packed_size
                     (const HeroMsg   *message);
size_t hero_msg__pack
                     (const HeroMsg   *message,
                      uint8_t             *out);
size_t hero_msg__pack_to_buffer
                     (const HeroMsg   *message,
                      ProtobufCBuffer     *buffer);
HeroMsg *
       hero_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   hero_msg__free_unpacked
                     (HeroMsg *message,
                      ProtobufCAllocator *allocator);
/* EnemyMsg methods */
void   enemy_msg__init
                     (EnemyMsg         *message);
size_t enemy_msg__get_packed_size
                     (const EnemyMsg   *message);
size_t enemy_msg__pack
                     (const EnemyMsg   *message,
                      uint8_t             *out);
size_t enemy_msg__pack_to_buffer
                     (const EnemyMsg   *message,
                      ProtobufCBuffer     *buffer);
EnemyMsg *
       enemy_msg__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   enemy_msg__free_unpacked
                     (EnemyMsg *message,
                      ProtobufCAllocator *allocator);
/* NewEnemy methods */
void   new_enemy__init
                     (NewEnemy         *message);
size_t new_enemy__get_packed_size
                     (const NewEnemy   *message);
size_t new_enemy__pack
                     (const NewEnemy   *message,
                      uint8_t             *out);
size_t new_enemy__pack_to_buffer
                     (const NewEnemy   *message,
                      ProtobufCBuffer     *buffer);
NewEnemy *
       new_enemy__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   new_enemy__free_unpacked
                     (NewEnemy *message,
                      ProtobufCAllocator *allocator);
/* EnemyLeave methods */
void   enemy_leave__init
                     (EnemyLeave         *message);
size_t enemy_leave__get_packed_size
                     (const EnemyLeave   *message);
size_t enemy_leave__pack
                     (const EnemyLeave   *message,
                      uint8_t             *out);
size_t enemy_leave__pack_to_buffer
                     (const EnemyLeave   *message,
                      ProtobufCBuffer     *buffer);
EnemyLeave *
       enemy_leave__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   enemy_leave__free_unpacked
                     (EnemyLeave *message,
                      ProtobufCAllocator *allocator);
/* LoginReq methods */
void   login_req__init
                     (LoginReq         *message);
size_t login_req__get_packed_size
                     (const LoginReq   *message);
size_t login_req__pack
                     (const LoginReq   *message,
                      uint8_t             *out);
size_t login_req__pack_to_buffer
                     (const LoginReq   *message,
                      ProtobufCBuffer     *buffer);
LoginReq *
       login_req__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   login_req__free_unpacked
                     (LoginReq *message,
                      ProtobufCAllocator *allocator);
/* LoginRsp methods */
void   login_rsp__init
                     (LoginRsp         *message);
size_t login_rsp__get_packed_size
                     (const LoginRsp   *message);
size_t login_rsp__pack
                     (const LoginRsp   *message,
                      uint8_t             *out);
size_t login_rsp__pack_to_buffer
                     (const LoginRsp   *message,
                      ProtobufCBuffer     *buffer);
LoginRsp *
       login_rsp__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   login_rsp__free_unpacked
                     (LoginRsp *message,
                      ProtobufCAllocator *allocator);
/* StartReq methods */
void   start_req__init
                     (StartReq         *message);
size_t start_req__get_packed_size
                     (const StartReq   *message);
size_t start_req__pack
                     (const StartReq   *message,
                      uint8_t             *out);
size_t start_req__pack_to_buffer
                     (const StartReq   *message,
                      ProtobufCBuffer     *buffer);
StartReq *
       start_req__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   start_req__free_unpacked
                     (StartReq *message,
                      ProtobufCAllocator *allocator);
/* StartRsp methods */
void   start_rsp__init
                     (StartRsp         *message);
size_t start_rsp__get_packed_size
                     (const StartRsp   *message);
size_t start_rsp__pack
                     (const StartRsp   *message,
                      uint8_t             *out);
size_t start_rsp__pack_to_buffer
                     (const StartRsp   *message,
                      ProtobufCBuffer     *buffer);
StartRsp *
       start_rsp__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   start_rsp__free_unpacked
                     (StartRsp *message,
                      ProtobufCAllocator *allocator);
/* LoginEnd methods */
void   login_end__init
                     (LoginEnd         *message);
size_t login_end__get_packed_size
                     (const LoginEnd   *message);
size_t login_end__pack
                     (const LoginEnd   *message,
                      uint8_t             *out);
size_t login_end__pack_to_buffer
                     (const LoginEnd   *message,
                      ProtobufCBuffer     *buffer);
LoginEnd *
       login_end__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   login_end__free_unpacked
                     (LoginEnd *message,
                      ProtobufCAllocator *allocator);
/* LeaveReq methods */
void   leave_req__init
                     (LeaveReq         *message);
size_t leave_req__get_packed_size
                     (const LeaveReq   *message);
size_t leave_req__pack
                     (const LeaveReq   *message,
                      uint8_t             *out);
size_t leave_req__pack_to_buffer
                     (const LeaveReq   *message,
                      ProtobufCBuffer     *buffer);
LeaveReq *
       leave_req__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   leave_req__free_unpacked
                     (LeaveReq *message,
                      ProtobufCAllocator *allocator);
/* LeaveRsp methods */
void   leave_rsp__init
                     (LeaveRsp         *message);
size_t leave_rsp__get_packed_size
                     (const LeaveRsp   *message);
size_t leave_rsp__pack
                     (const LeaveRsp   *message,
                      uint8_t             *out);
size_t leave_rsp__pack_to_buffer
                     (const LeaveRsp   *message,
                      ProtobufCBuffer     *buffer);
LeaveRsp *
       leave_rsp__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   leave_rsp__free_unpacked
                     (LeaveRsp *message,
                      ProtobufCAllocator *allocator);
/* MoveReq methods */
void   move_req__init
                     (MoveReq         *message);
size_t move_req__get_packed_size
                     (const MoveReq   *message);
size_t move_req__pack
                     (const MoveReq   *message,
                      uint8_t             *out);
size_t move_req__pack_to_buffer
                     (const MoveReq   *message,
                      ProtobufCBuffer     *buffer);
MoveReq *
       move_req__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   move_req__free_unpacked
                     (MoveReq *message,
                      ProtobufCAllocator *allocator);
/* MoveRsp methods */
void   move_rsp__init
                     (MoveRsp         *message);
size_t move_rsp__get_packed_size
                     (const MoveRsp   *message);
size_t move_rsp__pack
                     (const MoveRsp   *message,
                      uint8_t             *out);
size_t move_rsp__pack_to_buffer
                     (const MoveRsp   *message,
                      ProtobufCBuffer     *buffer);
MoveRsp *
       move_rsp__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   move_rsp__free_unpacked
                     (MoveRsp *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*HeroMsg_Closure)
                 (const HeroMsg *message,
                  void *closure_data);
typedef void (*EnemyMsg_Closure)
                 (const EnemyMsg *message,
                  void *closure_data);
typedef void (*NewEnemy_Closure)
                 (const NewEnemy *message,
                  void *closure_data);
typedef void (*EnemyLeave_Closure)
                 (const EnemyLeave *message,
                  void *closure_data);
typedef void (*LoginReq_Closure)
                 (const LoginReq *message,
                  void *closure_data);
typedef void (*LoginRsp_Closure)
                 (const LoginRsp *message,
                  void *closure_data);
typedef void (*StartReq_Closure)
                 (const StartReq *message,
                  void *closure_data);
typedef void (*StartRsp_Closure)
                 (const StartRsp *message,
                  void *closure_data);
typedef void (*LoginEnd_Closure)
                 (const LoginEnd *message,
                  void *closure_data);
typedef void (*LeaveReq_Closure)
                 (const LeaveReq *message,
                  void *closure_data);
typedef void (*LeaveRsp_Closure)
                 (const LeaveRsp *message,
                  void *closure_data);
typedef void (*MoveReq_Closure)
                 (const MoveReq *message,
                  void *closure_data);
typedef void (*MoveRsp_Closure)
                 (const MoveRsp *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor hero_msg__descriptor;
extern const ProtobufCMessageDescriptor enemy_msg__descriptor;
extern const ProtobufCMessageDescriptor new_enemy__descriptor;
extern const ProtobufCMessageDescriptor enemy_leave__descriptor;
extern const ProtobufCMessageDescriptor login_req__descriptor;
extern const ProtobufCMessageDescriptor login_rsp__descriptor;
extern const ProtobufCMessageDescriptor start_req__descriptor;
extern const ProtobufCMessageDescriptor start_rsp__descriptor;
extern const ProtobufCMessageDescriptor login_end__descriptor;
extern const ProtobufCMessageDescriptor leave_req__descriptor;
extern const ProtobufCMessageDescriptor leave_rsp__descriptor;
extern const ProtobufCMessageDescriptor move_req__descriptor;
extern const ProtobufCMessageDescriptor move_rsp__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_message_2eproto__INCLUDED */
