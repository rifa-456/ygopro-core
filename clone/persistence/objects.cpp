// SPDX-License-Identifier: AGPL-3.0-or-later
#include "archive.h"

void duel_archive::card_record(card &value) {
    lua_base(value);
    io(value.data);
    io(value.previous);
    io(value.temp);
    io(value.current);
    io(value.owner);
    io(value.summon);
    io(value.status);
    io(value.cover);
    io(value.sendto_param);
    io(value.release_param);
    io(value.sum_param);
    io(value.position_param);
    io(value.spsummon_param);
    io(value.to_field_param);
    io(value.attack_announce_count);
    io(value.direct_attackable);
    io(value.announce_count);
    io(value.attacked_count);
    io(value.attack_all_target);
    io(value.attack_controler);
    io(value.cardid);
    io(value.fieldid);
    io(value.fieldid_r);
    io(value.turnid);
    io(value.turn_counter);
    io(value.unique_pos);
    io(value.unique_fieldid);
    io(value.unique_code);
    io(value.unique_location);
    io(value.unique_function);
    io(value.unique_effect);
    io(value.spsummon_code);
    io(value.spsummon_counter);
    io(value.spsummon_counter_rst);
    io(value.assume);
    io(value.equiping_target);
    io(value.pre_equip_target);
    io(value.overlay_target);
    io(value.pre_overlay_target);
    io(value.relations);
    io(value.counters);
    io(value.indestructable_effects);
    io(value.announced_cards);
    io(value.attacked_cards);
    io(value.battled_cards);
    io(value.equiping_cards);
    io(value.material_cards);
    io(value.effect_target_owner);
    io(value.effect_target_cards);
    io(value.xyz_materials);
    io(value.single_effect);
    io(value.field_effect);
    io(value.equip_effect);
    io(value.target_effect);
    io(value.xmaterial_effect);
    io(value.relate_effect);
    io(value.immune_effect);
    if (stream_.reading)
        for (auto *collection : {&value.single_effect, &value.field_effect, &value.equip_effect,
                                 &value.target_effect, &value.xmaterial_effect})
            for (auto it = collection->begin(); it != collection->end(); ++it)
                value.indexer.emplace(it->second, it);
}
void duel_archive::effect_record(effect &value) {
    lua_base(value);
    io(value.count_limit);
    io(value.count_limit_max);
    io(value.count_flag);
    io(value.count_hopt_index);
    io(value.effect_owner);
    io(value.type);
    io(value.copy_id);
    io(value.range);
    io(value.s_range);
    io(value.o_range);
    io(value.reset_count);
    io(value.active_location);
    io(value.active_sequence);
    io(value.status);
    io(value.code);
    io(value.flag);
    io(value.id);
    io(value.initial_id);
    io(value.reset_flag);
    io(value.count_code);
    io(value.hint_timing);
    io(value.card_type);
    io(value.active_type);
    io(value.label_object);
    io(value.condition);
    io(value.cost);
    io(value.target);
    io(value.value);
    io(value.operation);
    io(value.category);
    io(value.owner);
    io(value.handler);
    io(value.active_handler);
    io(value.description);
    io(value.label);
}
