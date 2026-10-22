// SPDX-License-Identifier: AGPL-3.0-or-later
#include "copy.h"

void duel_clone::allocate_objects() {
    OCG_PROFILE_SCOPE(OCG_PROFILE_CORE_ALLOCATE);
    OCG_PROFILE_COUNT(OCG_PROFILE_CORE_CARDS, source.cards.size());
    OCG_PROFILE_COUNT(OCG_PROFILE_CORE_EFFECTS, source.effects.size());
    OCG_PROFILE_COUNT(OCG_PROFILE_CORE_GROUPS, source.groups.size());
    objects.emplace(&source,&destination);
    objects.emplace(source.lua, destination.lua);
    for(auto* s:source.cards) {
        auto d=std::unique_ptr<card>(new (destination.storage) card(&destination));
        static_cast<lua_obj&>(*d)=static_cast<const lua_obj&>(*s);
        d->pduel=&destination;
        d->cardid=s->cardid;
        objects.insert_or_assign(s,d.get());
        destination.cards.insert(d.get());
        d.release();
    }
    for(auto* s:source.effects) {
        auto d=std::unique_ptr<effect>(new (destination.storage) effect(&destination));
        static_cast<lua_obj&>(*d)=static_cast<const lua_obj&>(*s);
        d->pduel=&destination;
        d->id=s->id; d->initial_id=s->initial_id;
        objects.insert_or_assign(s,d.get());
        destination.effects.insert(d.get());
        d.release();
    }
    for(auto* s:source.groups) {
        auto d=std::unique_ptr<group>(new (destination.storage) group(&destination));
        static_cast<lua_obj&>(*d)=static_cast<const lua_obj&>(*s);
        d->pduel=&destination;
        objects.insert_or_assign(s,d.get());
        destination.groups.insert(d.get());
        d.release();
    }
    objects.emplace(&source.lua->deleted,&destination.lua->deleted);
}

void duel_clone::copy_objects() {
    OCG_PROFILE_SCOPE(OCG_PROFILE_CORE_OBJECTS);
    for(auto* s:source.cards) {
        auto* d=mapped(s);
        copy(d->data,s->data);
        copy(d->previous,s->previous);
        copy(d->temp,s->temp);
        copy(d->current,s->current);
        copy(d->owner,s->owner);
        copy(d->summon,s->summon);
        copy(d->status,s->status);
        copy(d->cover,s->cover);
        copy(d->sendto_param,s->sendto_param);
        copy(d->release_param,s->release_param);
        copy(d->sum_param,s->sum_param);
        copy(d->position_param,s->position_param);
        copy(d->spsummon_param,s->spsummon_param);
        copy(d->to_field_param,s->to_field_param);
        copy(d->attack_announce_count,s->attack_announce_count);
        copy(d->direct_attackable,s->direct_attackable);
        copy(d->announce_count,s->announce_count);
        copy(d->attacked_count,s->attacked_count);
        copy(d->attack_all_target,s->attack_all_target);
        copy(d->attack_controler,s->attack_controler);
        copy(d->cardid,s->cardid);
        copy(d->fieldid,s->fieldid);
        copy(d->fieldid_r,s->fieldid_r);
        copy(d->turnid,s->turnid);
        copy(d->turn_counter,s->turn_counter);
        copy(d->unique_pos,s->unique_pos);
        copy(d->unique_fieldid,s->unique_fieldid);
        copy(d->unique_code,s->unique_code);
        copy(d->unique_location,s->unique_location);
        copy(d->unique_function,s->unique_function);
        copy(d->unique_effect,s->unique_effect);
        copy(d->spsummon_code,s->spsummon_code);
        copy(d->spsummon_counter,s->spsummon_counter);
        copy(d->spsummon_counter_rst,s->spsummon_counter_rst);
        copy(d->assume,s->assume);
        copy(d->equiping_target,s->equiping_target);
        copy(d->pre_equip_target,s->pre_equip_target);
        copy(d->overlay_target,s->overlay_target);
        copy(d->pre_overlay_target,s->pre_overlay_target);
        copy(d->relations,s->relations);
        copy(d->counters,s->counters);
        copy(d->indestructable_effects,s->indestructable_effects);
        copy(d->announced_cards,s->announced_cards);
        copy(d->attacked_cards,s->attacked_cards);
        copy(d->battled_cards,s->battled_cards);
        copy(d->equiping_cards,s->equiping_cards);
        copy(d->material_cards,s->material_cards);
        copy(d->effect_target_owner,s->effect_target_owner);
        copy(d->effect_target_cards,s->effect_target_cards);
        copy(d->xyz_materials,s->xyz_materials);
        copy(d->single_effect,s->single_effect);
        copy(d->field_effect,s->field_effect);
        copy(d->equip_effect,s->equip_effect);
        copy(d->target_effect,s->target_effect);
        copy(d->xmaterial_effect,s->xmaterial_effect);
        copy(d->relate_effect,s->relate_effect);
        copy(d->immune_effect,s->immune_effect);
        for(auto* collection : {&d->single_effect,&d->field_effect,&d->equip_effect,&d->target_effect,&d->xmaterial_effect})
            for(auto it=collection->begin();it!=collection->end();++it) d->indexer.emplace(it->second,it);
    }
    for(auto* s:source.effects) {
        auto* d=mapped(s);
        copy(d->count_limit,s->count_limit);
        copy(d->count_limit_max,s->count_limit_max);
        copy(d->count_flag,s->count_flag);
        copy(d->count_hopt_index,s->count_hopt_index);
        copy(d->effect_owner,s->effect_owner);
        copy(d->type,s->type);
        copy(d->copy_id,s->copy_id);
        copy(d->range,s->range);
        copy(d->s_range,s->s_range);
        copy(d->o_range,s->o_range);
        copy(d->reset_count,s->reset_count);
        copy(d->active_location,s->active_location);
        copy(d->active_sequence,s->active_sequence);
        copy(d->status,s->status);
        copy(d->code,s->code);
        copy(d->flag,s->flag);
        copy(d->id,s->id);
        copy(d->initial_id,s->initial_id);
        copy(d->reset_flag,s->reset_flag);
        copy(d->count_code,s->count_code);
        copy(d->hint_timing,s->hint_timing);
        copy(d->card_type,s->card_type);
        copy(d->active_type,s->active_type);
        copy(d->label_object,s->label_object);
        copy(d->condition,s->condition);
        copy(d->cost,s->cost);
        copy(d->target,s->target);
        copy(d->value,s->value);
        copy(d->operation,s->operation);
        copy(d->category,s->category);
        copy(d->owner,s->owner);
        copy(d->handler,s->handler);
        copy(d->active_handler,s->active_handler);
        copy(d->description,s->description);
        copy(d->label,s->label);
    }
    for(auto* s:source.groups) {
        auto* d=mapped(s);
        copy(d->container,s->container);
        d->is_readonly=s->is_readonly;
        d->is_iterator_dirty=s->is_iterator_dirty;
        if(!s->is_iterator_dirty) d->it=s->it==s->container.end() ? d->container.end() : d->container.find(mapped(*s->it));
    }
}
